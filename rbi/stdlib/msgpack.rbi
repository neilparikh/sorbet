# This file is autogenerated. Do not edit it by hand. Regenerate it with:
#   scripts/bin/remote-script sorbet/shim_generation/gems.rb -r msgpack

# typed: true

module MessagePack
  DefaultFactory = ::T.let(nil, ::T.untyped)
  VERSION = ::T.let(nil, ::T.untyped)

  Sorbet.sig do
    params(
      v: ::T.untyped,
      rest: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def self.dump(v, *rest); end

  Sorbet.sig do
    params(
      src: ::T.untyped,
      param: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def self.load(src, param=T.unsafe(nil)); end

  Sorbet.sig do
    params(
      v: ::T.untyped,
      rest: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def self.pack(v, *rest); end

  Sorbet.sig do
    params(
      src: ::T.untyped,
      param: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def self.unpack(src, param=T.unsafe(nil)); end
end

class MessagePack::Buffer
  Sorbet.sig do
    params(
      _: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def <<(_); end

  Sorbet.sig {returns(::T.untyped)}
  def clear(); end

  Sorbet.sig {returns(::T.untyped)}
  def close(); end

  Sorbet.sig {returns(::T.untyped)}
  def empty?(); end

  Sorbet.sig {returns(::T.untyped)}
  def flush(); end

  Sorbet.sig do
    params(
      _: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def initialize(*_); end

  Sorbet.sig {returns(::T.untyped)}
  def io(); end

  Sorbet.sig do
    params(
      _: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def read(*_); end

  Sorbet.sig do
    params(
      _: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def read_all(*_); end

  Sorbet.sig {returns(::T.untyped)}
  def size(); end

  Sorbet.sig do
    params(
      _: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def skip(_); end

  Sorbet.sig do
    params(
      _: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def skip_all(_); end

  Sorbet.sig {returns(::T.untyped)}
  def to_a(); end

  Sorbet.sig {returns(::T.untyped)}
  def to_s(); end

  Sorbet.sig {returns(::T.untyped)}
  def to_str(); end

  Sorbet.sig do
    params(
      _: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def write(_); end

  Sorbet.sig do
    params(
      _: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def write_to(_); end
end

module MessagePack::CoreExt
  Sorbet.sig do
    params(
      packer_or_io: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def to_msgpack(packer_or_io=T.unsafe(nil)); end
end

class MessagePack::ExtensionValue < Struct
  include ::MessagePack::CoreExt
  Sorbet.sig {returns(::T.untyped)}
  def payload(); end

  Sorbet.sig do
    params(
      _: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def payload=(_); end

  Sorbet.sig {returns(::T.untyped)}
  def type(); end

  Sorbet.sig do
    params(
      _: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def type=(_); end

  Sorbet.sig do
    params(
      _: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def self.[](*_); end

  Sorbet.sig {returns(::T.untyped)}
  def self.members(); end

  Sorbet.sig do
    params(
      _: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def self.new(*_); end
end

class MessagePack::Factory
  Sorbet.sig do
    params(
      v: ::T.untyped,
      rest: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def dump(v, *rest); end

  Sorbet.sig do
    params(
      _: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def initialize(*_); end

  Sorbet.sig do
    params(
      src: ::T.untyped,
      param: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def load(src, param=T.unsafe(nil)); end

  Sorbet.sig do
    params(
      v: ::T.untyped,
      rest: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def pack(v, *rest); end

  Sorbet.sig do
    params(
      _: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def packer(*_); end

  Sorbet.sig do
    params(
      _: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def register_type(*_); end

  Sorbet.sig do
    params(
      selector: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def registered_types(selector=T.unsafe(nil)); end

  Sorbet.sig do
    params(
      klass_or_type: ::T.untyped,
      selector: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def type_registered?(klass_or_type, selector=T.unsafe(nil)); end

  Sorbet.sig do
    params(
      src: ::T.untyped,
      param: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def unpack(src, param=T.unsafe(nil)); end

  Sorbet.sig do
    params(
      _: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def unpacker(*_); end
end

class MessagePack::MalformedFormatError < MessagePack::UnpackError
end

class MessagePack::Packer
  Sorbet.sig {returns(::T.untyped)}
  def buffer(); end

  Sorbet.sig {returns(::T.untyped)}
  def clear(); end

  Sorbet.sig {returns(::T.untyped)}
  def compatibility_mode?(); end

  Sorbet.sig {returns(::T.untyped)}
  def empty?(); end

  Sorbet.sig {returns(::T.untyped)}
  def flush(); end

  Sorbet.sig {returns(::T.untyped)}
  def full_pack(); end

  Sorbet.sig do
    params(
      _: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def initialize(*_); end

  Sorbet.sig do
    params(
      _: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def pack(_); end

  Sorbet.sig do
    params(
      _: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def register_type(*_); end

  Sorbet.sig {returns(::T.untyped)}
  def registered_types(); end

  Sorbet.sig {returns(::T.untyped)}
  def size(); end

  Sorbet.sig {returns(::T.untyped)}
  def to_a(); end

  Sorbet.sig {returns(::T.untyped)}
  def to_s(); end

  Sorbet.sig {returns(::T.untyped)}
  def to_str(); end

  Sorbet.sig do
    params(
      klass_or_type: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def type_registered?(klass_or_type); end

  Sorbet.sig do
    params(
      _: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def write(_); end

  Sorbet.sig do
    params(
      _: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def write_array(_); end

  Sorbet.sig do
    params(
      _: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def write_array_header(_); end

  Sorbet.sig do
    params(
      _: ::T.untyped,
      _1: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def write_ext(_, _1); end

  Sorbet.sig do
    params(
      _: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def write_extension(_); end

  Sorbet.sig {returns(::T.untyped)}
  def write_false(); end

  Sorbet.sig do
    params(
      _: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def write_float(_); end

  Sorbet.sig do
    params(
      _: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def write_float32(_); end

  Sorbet.sig do
    params(
      _: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def write_hash(_); end

  Sorbet.sig do
    params(
      _: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def write_int(_); end

  Sorbet.sig do
    params(
      _: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def write_map_header(_); end

  Sorbet.sig {returns(::T.untyped)}
  def write_nil(); end

  Sorbet.sig do
    params(
      _: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def write_string(_); end

  Sorbet.sig do
    params(
      _: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def write_symbol(_); end

  Sorbet.sig do
    params(
      _: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def write_to(_); end

  Sorbet.sig {returns(::T.untyped)}
  def write_true(); end
end

class MessagePack::StackError < MessagePack::UnpackError
end

module MessagePack::TypeError
end

class MessagePack::UnexpectedTypeError < MessagePack::UnpackError
  include ::MessagePack::TypeError
end

class MessagePack::UnknownExtTypeError < MessagePack::UnpackError
end

class MessagePack::UnpackError < StandardError
end

class MessagePack::Unpacker
  Sorbet.sig {returns(::T.untyped)}
  def allow_unknown_ext?(); end

  Sorbet.sig {returns(::T.untyped)}
  def buffer(); end

  Sorbet.sig {returns(::T.untyped)}
  def each(); end

  Sorbet.sig do
    params(
      _: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def feed(_); end

  Sorbet.sig do
    params(
      _: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def feed_each(_); end

  Sorbet.sig {returns(::T.untyped)}
  def full_unpack(); end

  Sorbet.sig do
    params(
      _: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def initialize(*_); end

  Sorbet.sig {returns(::T.untyped)}
  def read(); end

  Sorbet.sig {returns(::T.untyped)}
  def read_array_header(); end

  Sorbet.sig {returns(::T.untyped)}
  def read_map_header(); end

  Sorbet.sig do
    params(
      _: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def register_type(*_); end

  Sorbet.sig {returns(::T.untyped)}
  def registered_types(); end

  Sorbet.sig {returns(::T.untyped)}
  def reset(); end

  Sorbet.sig {returns(::T.untyped)}
  def skip(); end

  Sorbet.sig {returns(::T.untyped)}
  def skip_nil(); end

  Sorbet.sig {returns(::T.untyped)}
  def symbolize_keys?(); end

  Sorbet.sig do
    params(
      klass_or_type: ::T.untyped,
    )
    .returns(::T.untyped)
  end
  def type_registered?(klass_or_type); end

  Sorbet.sig {returns(::T.untyped)}
  def unpack(); end
end