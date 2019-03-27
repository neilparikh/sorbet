#!/bin/bash

set -exuo pipefail

whitelisted=0

if [[ "$BUILDKITE_PULL_REQUEST" == "false" ]]; then
  # whitelist commits that are triggered in branch builds of github.com/stripe/sorbet
  whitelisted=1
fi

if [[ "$BUILDKITE_PULL_REQUEST_REPO" == "git://github.com/stripe/sorbet.git" ]]; then
  # whitelist folks with write access to github.com/stripe/sorbet
  whitelisted=1
fi

if [[ "${whitelisted}" -ne 1 ]] ; then
   (echo -e "steps:\n  - block: \":key: Needs contributor approval!\"\n  - wait: ~\n";
   cat .buildkite/pipeline.yaml | grep -v "steps:") | buildkite-agent pipeline upload
else
  buildkite-agent pipeline upload
fi