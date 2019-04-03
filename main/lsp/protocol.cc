#include "absl/synchronization/mutex.h"
#include "common/Timer.h"
#include "lsp.h"
#include "main/lsp/watchman/WatchmanProcess.h"
#include "main/options/options.h" // For EarlyReturnWithCode.
#include <iostream>

using namespace std;

namespace sorbet::realmain::lsp {

bool safeGetline(istream &is, string &t) {
    t.clear();

    // The characters in the stream are read one-by-one using a streambuf.
    // That is faster than reading them one-by-one using the istream.
    // Code that uses streambuf this way must be guarded by a sentry object.
    // The sentry object performs various tasks,
    // such as thread synchronization and updating the stream state.

    istream::sentry se(is, true);
    streambuf *sb = is.rdbuf();

    for (;;) {
        int c = sb->sbumpc();
        switch (c) {
            case '\n':
                return true;
            case '\r':
                if (sb->sgetc() == '\n')
                    sb->sbumpc();
                return true;
            case streambuf::traits_type::eof():
                // Also handle the case when the last line has no line ending
                if (t.empty())
                    is.setstate(ios::eofbit);
                return false;
            default:
                t += (char)c;
        }
    }
}

bool getNewRequest(rapidjson::Document &d, const shared_ptr<spd::logger> &logger, istream &inputStream) {
    int length = -1;
    {
        string line;
        while (safeGetline(inputStream, line)) {
            logger->trace("raw read: {}", line);
            if (line == "") {
                break;
            }
            sscanf(line.c_str(), "Content-Length: %i", &length);
        }
        logger->trace("final raw read: {}, length: {}", line, length);
    }
    if (length < 0) {
        logger->debug("eof or no \"Content-Length: %i\" header found.");
        return false;
    }

    string json(length, '\0');
    inputStream.read(&json[0], length);
    logger->debug("Read: {}", json);
    if (d.Parse(json.c_str()).HasParseError()) {
        logger->error("Last LSP request: `{}` is not a valid json object", json);
        return false;
    }
    return true;
}

class NotifyOnDestruction {
    absl::Mutex &mutex;
    bool &flag;

public:
    NotifyOnDestruction(absl::Mutex &mutex, bool &flag) : mutex(mutex), flag(flag){};
    ~NotifyOnDestruction() {
        absl::MutexLock lck(&mutex);
        flag = true;
    }
};

unique_ptr<core::GlobalState> LSPLoop::runLSP() {
    // Naming convention: thread that executes this function is called coordinator thread
    LSPLoop::QueueState guardedState{{}, false, false, 0};
    absl::Mutex mtx;

    unique_ptr<watchman::WatchmanProcess> watchmanProcess;
    if (!opts.disableWatchman) {
        if (opts.rawInputDirNames.size() == 1 && opts.rawInputFileNames.size() == 0) {
            // The lambda below intentionally does not capture `this`.
            watchmanProcess = make_unique<watchman::WatchmanProcess>(
                logger, opts.rawInputDirNames.at(0), vector<string>({"rb", "rbi"}),
                [&guardedState, &mtx, logger = this->logger](rapidjson::MemoryPoolAllocator<> &alloc,
                                                             const WatchmanQueryResponse &response) {
                    // TODO(jvilk): We are wastefully parsing from JSON + serializing to JSON value + parsing from JSON
                    // value. Once we stop using JSON values internally, we can make this less wasteful.
                    auto notifMsg = make_unique<NotificationMessage>("2.0", "sorbet/watchmanFileChange");
                    notifMsg->params = response.toJSONValue(alloc);
                    auto msg = make_unique<LSPMessage>(move(notifMsg));
                    {
                        absl::MutexLock lck(&mtx); // guards guardedState
                        // Merge with any existing pending watchman file updates.
                        enqueueRequest(alloc, logger, guardedState, move(msg), true);
                    }
                },
                [&guardedState, &mtx, logger = this->logger](rapidjson::MemoryPoolAllocator<> &alloc,
                                                             int watchmanExitCode) {
                    auto notifMsg = make_unique<NotificationMessage>("2.0", "sorbet/watchmanExit");
                    notifMsg->params = make_unique<rapidjson::Value>(watchmanExitCode);
                    auto msg = make_unique<LSPMessage>(move(notifMsg));
                    {
                        absl::MutexLock lck(&mtx); // guards guardedState
                        enqueueRequest(alloc, logger, guardedState, move(msg), true);
                    }
                });
        } else {
            logger->error("Watchman support currently only works when Sorbet is run with a single input directory. If "
                          "Watchman is not needed, run Sorbet with `--disable-watchman`.");
            throw options::EarlyReturnWithCode(1);
        }
    }

    // !!DO NOT USE OUTSIDE OF READER THREAD!!
    // We need objects created by the reader thread to outlive the thread itself. At the same time, MemoryPoolAllocator
    // is not thread-safe.
    // TODO(jvilk): This (+ Watchman's alloc) leak memory. If we stop using JSON values internally, then we can clear
    // them after every request and only use them for intermediate objects generated during parsing.
    rapidjson::MemoryPoolAllocator<> readerAlloc;
    auto readerThread = runInAThread(
        "lspReader", [&guardedState, &mtx, logger = this->logger, &readerAlloc, &inputStream = this->inputStream] {
            // Thread that executes this lambda is called reader thread.
            // This thread _intentionally_ does not capture `this`.
            NotifyOnDestruction notify(mtx, guardedState.terminate);
            while (true) {
                rapidjson::Document d(&readerAlloc);

                if (!getNewRequest(d, logger, inputStream)) {
                    break;
                }
                unique_ptr<LSPMessage> msg;
                try {
                    msg = make_unique<LSPMessage>(readerAlloc, d);
                } catch (DeserializationError e) {
                    logger->error(fmt::format("Unable to deserialize LSP request: {}", e.what()));
                }

                if (msg) {
                    absl::MutexLock lck(&mtx); // guards guardedState.
                    enqueueRequest(readerAlloc, logger, guardedState, move(msg), true);
                }
            }
        });

    mainThreadId = this_thread::get_id();
    unique_ptr<core::GlobalState> gs;
    while (true) {
        unique_ptr<LSPMessage> msg;
        {
            absl::MutexLock lck(&mtx);
            Timer timeit(logger, "idle");
            mtx.Await(absl::Condition(
                +[](LSPLoop::QueueState *guardedState) -> bool {
                    return guardedState->terminate || (!guardedState->paused && !guardedState->pendingRequests.empty());
                },
                &guardedState));
            ENFORCE(!guardedState.paused);
            if (guardedState.terminate) {
                if (guardedState.errorCode > 0) {
                    // Abnormal termination.
                    throw options::EarlyReturnWithCode(guardedState.errorCode);
                }
                break;
            }
            msg = move(guardedState.pendingRequests.front());
            guardedState.pendingRequests.pop_front();
        }
        prodCounterInc("lsp.messages.received");
        auto requestReceiveTime = msg->timestamp;
        gs = processRequest(move(gs), *msg);
        auto currentTime = chrono::steady_clock::now();
        auto processingTime = currentTime - requestReceiveTime;
        auto processingTime_ns = chrono::duration_cast<chrono::nanoseconds>(processingTime);
        timingAdd("processing_time", processingTime_ns.count());
        if (shouldSendCountersToStatsd(currentTime)) {
            {
                // Merge counters from worker threads.
                absl::MutexLock counterLck(&mtx);
                if (!guardedState.counters.hasNullCounters()) {
                    counterConsume(move(guardedState.counters));
                }
            }
            sendCountersToStatsd(currentTime);
        }
    }

    if (gs) {
        return gs;
    } else {
        return move(initialGS);
    }
}

void LSPLoop::mergeFileChanges(rapidjson::MemoryPoolAllocator<> &alloc,
                               deque<unique_ptr<LSPMessage>> &pendingRequests) {
    // Squish any consecutive didChanges that are for the same file together, and combine all Watchman file system
    // updates into a single update.
    // TODO: if we ever support diffs, this would need to be extended
    int didChangeRequestsMerged = 0;
    int foundWatchmanRequests = 0;
    chrono::time_point<chrono::steady_clock> firstWatchmanTimestamp;
    int firstWatchmanCounter;
    int originalSize = pendingRequests.size();
    UnorderedSet<string> updatedFiles;
    for (auto it = pendingRequests.begin(); it != pendingRequests.end();) {
        auto &current = *it;
        auto method = LSPMethod::getByName(current->method());
        if (method == LSPMethod::TextDocumentDidChange()) {
            auto currentChanges = DidChangeTextDocumentParams::fromJSONValue(alloc, current->params(), "root.params");
            string_view thisURI = currentChanges->textDocument->uri;
            auto nextIt = it + 1;
            if (nextIt != pendingRequests.end()) {
                auto &next = *nextIt;
                auto nextMethod = LSPMethod::getByName(next->method());
                if (nextMethod == LSPMethod::TextDocumentDidChange()) {
                    auto nextChanges = DidChangeTextDocumentParams::fromJSONValue(alloc, next->params(), "root.params");
                    string_view nextURI = nextChanges->textDocument->uri;
                    if (nextURI == thisURI) {
                        auto &currentUpdates = currentChanges->contentChanges;
                        auto &nextUpdates = nextChanges->contentChanges;
                        std::move(std::begin(nextUpdates), std::end(nextUpdates), std::back_inserter(currentUpdates));
                        nextChanges->contentChanges = move(currentUpdates);
                        didChangeRequestsMerged += 1;
                        next->setParams(nextChanges->toJSONValue(alloc));
                        it = pendingRequests.erase(it);
                        continue;
                    }
                }
            }
        } else if (method == LSPMethod::SorbetWatchmanFileChange()) {
            auto changes = WatchmanQueryResponse::fromJSONValue(alloc, current->params(), "root.params");
            updatedFiles.insert(changes->files.begin(), changes->files.end());
            if (foundWatchmanRequests == 0) {
                // Use timestamp/counter from the earliest file system change.
                firstWatchmanTimestamp = current->timestamp;
                firstWatchmanCounter = current->counter;
            }
            foundWatchmanRequests++;
            it = pendingRequests.erase(it);
            continue;
        }
        ++it;
    }
    ENFORCE(pendingRequests.size() + didChangeRequestsMerged + foundWatchmanRequests == originalSize);

    prodCategoryCounterAdd("lsp.messages.processed", "text_document_did_change_merged", didChangeRequestsMerged);

    /**
     * If we found any watchman updates, inject a single update that contains all of the FS updates.
     *
     * Ordering with textDocumentDidChange notifications effectively does *not* matter. We ignore Watchman updates on
     * files that are open in the editor, and re-pick up the file system version when the editor closes the file.
     *
     * It's possible that this reordering will cause transient errors if the user intended the TextDocumentDidChange to
     * apply against the other changed files on disk. For example, the user may have git pulled in changes to bar.rb,
     * and are editing foo.rb which requires the updated bar.rb to pass typechecking. However, we already have that
     * problem with the devbox rsync loop + autogen latency.
     *
     * I (jvilk) think the harm of these transient errors is small compared to the potential delay in this common
     * scenario:
     * 1. User runs git pull.
     * 2. User begins editing file.
     * 3. While user is editing file, pay up is rsyncing changes + autogen is running.
     * 4. Sorbet is picking up user's edits mixed in with file system updates.
     *
     * If DidChange acts like a fence for combining file system updates, then Sorbet may have to run the slow path many
     * times to catch up with the file system changes.
     *
     * TODO(jvilk): Since the slow path is slow, I actually think that we may want to (eventually) combine all file
     * updates from different sources into a single event to make catching up faster. Then, we can merge all DidChange
     * and Watchman updates into a single update.
     */
    if (foundWatchmanRequests > 0) {
        prodCategoryCounterAdd("lsp.messages.processed", "watchman_file_change_merged", foundWatchmanRequests - 1);
        // Enqueue the changes at *back* of the queue. Defers Sorbet from processing updates until the last
        // possible moment.
        WatchmanQueryResponse watchmanUpdates("", "", false, vector<string>(updatedFiles.begin(), updatedFiles.end()));
        auto notifMsg = make_unique<NotificationMessage>("2.0", "sorbet/watchmanFileChange");
        notifMsg->params = watchmanUpdates.toJSONValue(alloc);
        auto msg = make_unique<LSPMessage>(move(notifMsg));
        msg->counter = firstWatchmanCounter;
        msg->timestamp = firstWatchmanTimestamp;
        pendingRequests.push_back(move(msg));
    }
}

void LSPLoop::enqueueRequest(rapidjson::MemoryPoolAllocator<> &alloc, const shared_ptr<spd::logger> &logger,
                             LSPLoop::QueueState &state, std::unique_ptr<LSPMessage> msg, bool collectThreadCounters) {
    try {
        msg->counter = state.requestCounter++;
        msg->timestamp = chrono::steady_clock::now();

        const LSPMethod method = LSPMethod::getByName(msg->method());
        if (method == LSPMethod::CancelRequest()) {
            // see if they are canceling request that we didn't yet even start processing.
            auto it = findRequestToBeCancelled(state.pendingRequests,
                                               *CancelParams::fromJSONValue(alloc, msg->params(), "root.params"));
            if (it != state.pendingRequests.end() && (*it)->isRequest()) {
                auto canceledRequest = move(*it);
                canceledRequest->canceled = true;
                state.pendingRequests.erase(it);
                // move the canceled request to the front
                auto itFront = findFirstPositionAfterLSPInitialization(state.pendingRequests);
                state.pendingRequests.insert(itFront, move(canceledRequest));
                LSPLoop::mergeFileChanges(alloc, state.pendingRequests);
            }
            // if we started processing it already, well... swallow the cancellation request and
            // continue computing.
        } else if (method == LSPMethod::Pause()) {
            ENFORCE(!state.paused);
            logger->error("Pausing");
            state.paused = true;
        } else if (method == LSPMethod::Resume()) {
            logger->error("Resuming");
            ENFORCE(state.paused);
            state.paused = false;
        } else if (method == LSPMethod::SorbetWatchmanExit()) {
            state.terminate = true;
            state.errorCode = msg->params().GetInt();
        } else {
            state.pendingRequests.emplace_back(move(msg));
            LSPLoop::mergeFileChanges(alloc, state.pendingRequests);
        }
    } catch (DeserializationError e) {
        logger->error(fmt::format("Unable to deserialize LSP request: {}", e.what()));
    }

    if (collectThreadCounters) {
        if (!state.counters.hasNullCounters()) {
            counterConsume(move(state.counters));
        }
        state.counters = getAndClearThreadCounters();
    }
}

std::deque<std::unique_ptr<LSPMessage>>::iterator
LSPLoop::findRequestToBeCancelled(std::deque<std::unique_ptr<LSPMessage>> &pendingRequests,
                                  const CancelParams &cancelParams) {
    for (auto it = pendingRequests.begin(); it != pendingRequests.end(); ++it) {
        auto &current = *it;
        if (current->isRequest()) {
            auto &request = current->asRequest();
            if (request.id == cancelParams.id) {
                return it;
            }
        }
    }
    return pendingRequests.end();
}

std::deque<std::unique_ptr<LSPMessage>>::iterator
LSPLoop::findFirstPositionAfterLSPInitialization(std::deque<std::unique_ptr<LSPMessage>> &pendingRequests) {
    for (auto it = pendingRequests.begin(); it != pendingRequests.end(); ++it) {
        auto &current = *it;
        auto method = LSPMethod::getByName(current->method());
        if (method != LSPMethod::LSPMethod::Initialize() && method != LSPMethod::LSPMethod::Initialized()) {
            return it;
        }
    }
    return pendingRequests.end();
}

void LSPLoop::sendShowMessageNotification(MessageType messageType, string_view message) {
    sendNotification(LSPMethod::WindowShowMessage(), ShowMessageParams(messageType, string(message)));
}

void LSPLoop::sendNullResponse(const MessageId &id) {
    auto resp = ResponseMessage("2.0", id);
    // rapidjson values default to null.
    resp.result = make_unique<rapidjson::Value>();
    sendRaw(resp.toJSON());
}

void LSPLoop::sendResponse(const MessageId &id, const JSONBaseType &result) {
    auto resp = ResponseMessage("2.0", id);
    resp.result = result.toJSONValue(alloc);
    sendRaw(resp.toJSON());
}

void LSPLoop::sendResponse(const MessageId &id, const vector<unique_ptr<JSONBaseType>> &result) {
    auto finalResult = make_unique<rapidjson::Value>();
    finalResult->SetArray();
    for (auto &item : result) {
        finalResult->PushBack(*item->toJSONValue(alloc), alloc);
    }
    auto resp = ResponseMessage("2.0", id);
    resp.result = move(finalResult);
    sendRaw(resp.toJSON());
}

void LSPLoop::sendError(const MessageId &id, unique_ptr<ResponseError> error) {
    auto resp = ResponseMessage("2.0", id);
    resp.error = move(error);
    sendRaw(resp.toJSON());
}

void LSPLoop::sendError(const MessageId &id, int errorCode, string_view errorMsg) {
    sendError(id, make_unique<ResponseError>(errorCode, string(errorMsg)));
}

unique_ptr<core::Loc> LSPLoop::lspPos2Loc(core::FileRef fref, const Position &pos, const core::GlobalState &gs) {
    core::Loc::Detail reqPos;
    reqPos.line = pos.line + 1;
    reqPos.column = pos.character + 1;
    auto offset = core::Loc::pos2Offset(fref.data(gs), reqPos);
    return make_unique<core::Loc>(core::Loc(fref, offset, offset));
}

bool LSPLoop::handleReplies(const LSPMessage &msg) {
    if (msg.isResponse()) {
        auto &resp = msg.asResponse();
        auto id = resp.id;
        if (auto stringId = get_if<string>(&id)) {
            auto fnd = awaitingResponse.find(*stringId);
            if (fnd != awaitingResponse.end()) {
                auto func = move(fnd->second.onResult);
                awaitingResponse.erase(fnd);

                if (resp.error.has_value()) {
                    auto &error = *resp.error;
                    func(*error->toJSONValue(alloc));
                } else if (resp.result.has_value()) {
                    auto &result = *resp.result;
                    func(*result);
                }
            }
        }
        return true;
    }
    return false;
}

void LSPLoop::sendRaw(string_view json) {
    string outResult = fmt::format("Content-Length: {}\r\n\r\n{}", json.length(), json);
    logger->debug("Write: {}\n", json);
    outputStream << outResult << flush;
}

void LSPLoop::sendNotification(LSPMethod meth, const JSONBaseType &data) {
    ENFORCE(meth.isNotification);
    ENFORCE(meth.kind == LSPMethod::Kind::ServerInitiated || meth.kind == LSPMethod::Kind::Both);

    auto notif = NotificationMessage("2.0", meth.name);
    notif.params = data.toJSONValue(alloc);

    sendRaw(notif.toJSON());
}

} // namespace sorbet::realmain::lsp