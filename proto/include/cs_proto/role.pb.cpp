// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: cs_proto/role.proto

#include "cs_proto/role.pb.h"

#include <algorithm>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/wire_format_lite.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>

PROTOBUF_PRAGMA_INIT_SEG

namespace _pb = ::PROTOBUF_NAMESPACE_ID;
namespace _pbi = _pb::internal;

namespace cspkg {
PROTOBUF_CONSTEXPR AccountLoginReq::AccountLoginReq(
    ::_pbi::ConstantInitialized): _impl_{
    /*decltype(_impl_.session_key_)*/{&::_pbi::fixed_address_empty_string, ::_pbi::ConstantInitialized{}}
  , /*decltype(_impl_.account_)*/0u
  , /*decltype(_impl_._cached_size_)*/{}} {}
struct AccountLoginReqDefaultTypeInternal {
  PROTOBUF_CONSTEXPR AccountLoginReqDefaultTypeInternal()
      : _instance(::_pbi::ConstantInitialized{}) {}
  ~AccountLoginReqDefaultTypeInternal() {}
  union {
    AccountLoginReq _instance;
  };
};
PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 AccountLoginReqDefaultTypeInternal _AccountLoginReq_default_instance_;
PROTOBUF_CONSTEXPR AccountLoginRes::AccountLoginRes(
    ::_pbi::ConstantInitialized): _impl_{
    /*decltype(_impl_.account_)*/0u
  , /*decltype(_impl_.result_)*/0
  , /*decltype(_impl_._cached_size_)*/{}} {}
struct AccountLoginResDefaultTypeInternal {
  PROTOBUF_CONSTEXPR AccountLoginResDefaultTypeInternal()
      : _instance(::_pbi::ConstantInitialized{}) {}
  ~AccountLoginResDefaultTypeInternal() {}
  union {
    AccountLoginRes _instance;
  };
};
PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 AccountLoginResDefaultTypeInternal _AccountLoginRes_default_instance_;
}  // namespace cspkg
static ::_pb::Metadata file_level_metadata_cs_5fproto_2frole_2eproto[2];
static constexpr ::_pb::EnumDescriptor const** file_level_enum_descriptors_cs_5fproto_2frole_2eproto = nullptr;
static constexpr ::_pb::ServiceDescriptor const** file_level_service_descriptors_cs_5fproto_2frole_2eproto = nullptr;

const uint32_t TableStruct_cs_5fproto_2frole_2eproto::offsets[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  ~0u,  // no _has_bits_
  PROTOBUF_FIELD_OFFSET(::cspkg::AccountLoginReq, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  ~0u,  // no _inlined_string_donated_
  PROTOBUF_FIELD_OFFSET(::cspkg::AccountLoginReq, _impl_.account_),
  PROTOBUF_FIELD_OFFSET(::cspkg::AccountLoginReq, _impl_.session_key_),
  ~0u,  // no _has_bits_
  PROTOBUF_FIELD_OFFSET(::cspkg::AccountLoginRes, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  ~0u,  // no _inlined_string_donated_
  PROTOBUF_FIELD_OFFSET(::cspkg::AccountLoginRes, _impl_.account_),
  PROTOBUF_FIELD_OFFSET(::cspkg::AccountLoginRes, _impl_.result_),
};
static const ::_pbi::MigrationSchema schemas[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  { 0, -1, -1, sizeof(::cspkg::AccountLoginReq)},
  { 8, -1, -1, sizeof(::cspkg::AccountLoginRes)},
};

static const ::_pb::Message* const file_default_instances[] = {
  &::cspkg::_AccountLoginReq_default_instance_._instance,
  &::cspkg::_AccountLoginRes_default_instance_._instance,
};

const char descriptor_table_protodef_cs_5fproto_2frole_2eproto[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) =
  "\n\023cs_proto/role.proto\022\005cspkg\"7\n\017AccountL"
  "oginReq\022\017\n\007account\030\001 \001(\007\022\023\n\013session_key\030"
  "\002 \001(\t\"2\n\017AccountLoginRes\022\017\n\007account\030\001 \001("
  "\007\022\016\n\006result\030\002 \001(\005b\006proto3"
  ;
static ::_pbi::once_flag descriptor_table_cs_5fproto_2frole_2eproto_once;
const ::_pbi::DescriptorTable descriptor_table_cs_5fproto_2frole_2eproto = {
    false, false, 145, descriptor_table_protodef_cs_5fproto_2frole_2eproto,
    "cs_proto/role.proto",
    &descriptor_table_cs_5fproto_2frole_2eproto_once, nullptr, 0, 2,
    schemas, file_default_instances, TableStruct_cs_5fproto_2frole_2eproto::offsets,
    file_level_metadata_cs_5fproto_2frole_2eproto, file_level_enum_descriptors_cs_5fproto_2frole_2eproto,
    file_level_service_descriptors_cs_5fproto_2frole_2eproto,
};
PROTOBUF_ATTRIBUTE_WEAK const ::_pbi::DescriptorTable* descriptor_table_cs_5fproto_2frole_2eproto_getter() {
  return &descriptor_table_cs_5fproto_2frole_2eproto;
}

// Force running AddDescriptors() at dynamic initialization time.
PROTOBUF_ATTRIBUTE_INIT_PRIORITY2 static ::_pbi::AddDescriptorsRunner dynamic_init_dummy_cs_5fproto_2frole_2eproto(&descriptor_table_cs_5fproto_2frole_2eproto);
namespace cspkg {

// ===================================================================

class AccountLoginReq::_Internal {
 public:
};

AccountLoginReq::AccountLoginReq(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                         bool is_message_owned)
  : ::PROTOBUF_NAMESPACE_ID::Message(arena, is_message_owned) {
  SharedCtor(arena, is_message_owned);
  // @@protoc_insertion_point(arena_constructor:cspkg.AccountLoginReq)
}
AccountLoginReq::AccountLoginReq(const AccountLoginReq& from)
  : ::PROTOBUF_NAMESPACE_ID::Message() {
  AccountLoginReq* const _this = this; (void)_this;
  new (&_impl_) Impl_{
      decltype(_impl_.session_key_){}
    , decltype(_impl_.account_){}
    , /*decltype(_impl_._cached_size_)*/{}};

  _internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
  _impl_.session_key_.InitDefault();
  #ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
    _impl_.session_key_.Set("", GetArenaForAllocation());
  #endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (!from._internal_session_key().empty()) {
    _this->_impl_.session_key_.Set(from._internal_session_key(), 
      _this->GetArenaForAllocation());
  }
  _this->_impl_.account_ = from._impl_.account_;
  // @@protoc_insertion_point(copy_constructor:cspkg.AccountLoginReq)
}

inline void AccountLoginReq::SharedCtor(
    ::_pb::Arena* arena, bool is_message_owned) {
  (void)arena;
  (void)is_message_owned;
  new (&_impl_) Impl_{
      decltype(_impl_.session_key_){}
    , decltype(_impl_.account_){0u}
    , /*decltype(_impl_._cached_size_)*/{}
  };
  _impl_.session_key_.InitDefault();
  #ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
    _impl_.session_key_.Set("", GetArenaForAllocation());
  #endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
}

AccountLoginReq::~AccountLoginReq() {
  // @@protoc_insertion_point(destructor:cspkg.AccountLoginReq)
  if (auto *arena = _internal_metadata_.DeleteReturnArena<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>()) {
  (void)arena;
    return;
  }
  SharedDtor();
}

inline void AccountLoginReq::SharedDtor() {
  GOOGLE_DCHECK(GetArenaForAllocation() == nullptr);
  _impl_.session_key_.Destroy();
}

void AccountLoginReq::SetCachedSize(int size) const {
  _impl_._cached_size_.Set(size);
}

void AccountLoginReq::Clear() {
// @@protoc_insertion_point(message_clear_start:cspkg.AccountLoginReq)
  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  _impl_.session_key_.ClearToEmpty();
  _impl_.account_ = 0u;
  _internal_metadata_.Clear<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
}

const char* AccountLoginReq::_InternalParse(const char* ptr, ::_pbi::ParseContext* ctx) {
#define CHK_(x) if (PROTOBUF_PREDICT_FALSE(!(x))) goto failure
  while (!ctx->Done(&ptr)) {
    uint32_t tag;
    ptr = ::_pbi::ReadTag(ptr, &tag);
    switch (tag >> 3) {
      // fixed32 account = 1;
      case 1:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 13)) {
          _impl_.account_ = ::PROTOBUF_NAMESPACE_ID::internal::UnalignedLoad<uint32_t>(ptr);
          ptr += sizeof(uint32_t);
        } else
          goto handle_unusual;
        continue;
      // string session_key = 2;
      case 2:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 18)) {
          auto str = _internal_mutable_session_key();
          ptr = ::_pbi::InlineGreedyStringParser(str, ptr, ctx);
          CHK_(ptr);
          CHK_(::_pbi::VerifyUTF8(str, "cspkg.AccountLoginReq.session_key"));
        } else
          goto handle_unusual;
        continue;
      default:
        goto handle_unusual;
    }  // switch
  handle_unusual:
    if ((tag == 0) || ((tag & 7) == 4)) {
      CHK_(ptr);
      ctx->SetLastTag(tag);
      goto message_done;
    }
    ptr = UnknownFieldParse(
        tag,
        _internal_metadata_.mutable_unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(),
        ptr, ctx);
    CHK_(ptr != nullptr);
  }  // while
message_done:
  return ptr;
failure:
  ptr = nullptr;
  goto message_done;
#undef CHK_
}

uint8_t* AccountLoginReq::_InternalSerialize(
    uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const {
  // @@protoc_insertion_point(serialize_to_array_start:cspkg.AccountLoginReq)
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  // fixed32 account = 1;
  if (this->_internal_account() != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteFixed32ToArray(1, this->_internal_account(), target);
  }

  // string session_key = 2;
  if (!this->_internal_session_key().empty()) {
    ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::VerifyUtf8String(
      this->_internal_session_key().data(), static_cast<int>(this->_internal_session_key().length()),
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::SERIALIZE,
      "cspkg.AccountLoginReq.session_key");
    target = stream->WriteStringMaybeAliased(
        2, this->_internal_session_key(), target);
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    target = ::_pbi::WireFormat::InternalSerializeUnknownFieldsToArray(
        _internal_metadata_.unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(::PROTOBUF_NAMESPACE_ID::UnknownFieldSet::default_instance), target, stream);
  }
  // @@protoc_insertion_point(serialize_to_array_end:cspkg.AccountLoginReq)
  return target;
}

size_t AccountLoginReq::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:cspkg.AccountLoginReq)
  size_t total_size = 0;

  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  // string session_key = 2;
  if (!this->_internal_session_key().empty()) {
    total_size += 1 +
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::StringSize(
        this->_internal_session_key());
  }

  // fixed32 account = 1;
  if (this->_internal_account() != 0) {
    total_size += 1 + 4;
  }

  return MaybeComputeUnknownFieldsSize(total_size, &_impl_._cached_size_);
}

const ::PROTOBUF_NAMESPACE_ID::Message::ClassData AccountLoginReq::_class_data_ = {
    ::PROTOBUF_NAMESPACE_ID::Message::CopyWithSourceCheck,
    AccountLoginReq::MergeImpl
};
const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*AccountLoginReq::GetClassData() const { return &_class_data_; }


void AccountLoginReq::MergeImpl(::PROTOBUF_NAMESPACE_ID::Message& to_msg, const ::PROTOBUF_NAMESPACE_ID::Message& from_msg) {
  auto* const _this = static_cast<AccountLoginReq*>(&to_msg);
  auto& from = static_cast<const AccountLoginReq&>(from_msg);
  // @@protoc_insertion_point(class_specific_merge_from_start:cspkg.AccountLoginReq)
  GOOGLE_DCHECK_NE(&from, _this);
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  if (!from._internal_session_key().empty()) {
    _this->_internal_set_session_key(from._internal_session_key());
  }
  if (from._internal_account() != 0) {
    _this->_internal_set_account(from._internal_account());
  }
  _this->_internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
}

void AccountLoginReq::CopyFrom(const AccountLoginReq& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:cspkg.AccountLoginReq)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool AccountLoginReq::IsInitialized() const {
  return true;
}

void AccountLoginReq::InternalSwap(AccountLoginReq* other) {
  using std::swap;
  auto* lhs_arena = GetArenaForAllocation();
  auto* rhs_arena = other->GetArenaForAllocation();
  _internal_metadata_.InternalSwap(&other->_internal_metadata_);
  ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::InternalSwap(
      &_impl_.session_key_, lhs_arena,
      &other->_impl_.session_key_, rhs_arena
  );
  swap(_impl_.account_, other->_impl_.account_);
}

::PROTOBUF_NAMESPACE_ID::Metadata AccountLoginReq::GetMetadata() const {
  return ::_pbi::AssignDescriptors(
      &descriptor_table_cs_5fproto_2frole_2eproto_getter, &descriptor_table_cs_5fproto_2frole_2eproto_once,
      file_level_metadata_cs_5fproto_2frole_2eproto[0]);
}

// ===================================================================

class AccountLoginRes::_Internal {
 public:
};

AccountLoginRes::AccountLoginRes(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                         bool is_message_owned)
  : ::PROTOBUF_NAMESPACE_ID::Message(arena, is_message_owned) {
  SharedCtor(arena, is_message_owned);
  // @@protoc_insertion_point(arena_constructor:cspkg.AccountLoginRes)
}
AccountLoginRes::AccountLoginRes(const AccountLoginRes& from)
  : ::PROTOBUF_NAMESPACE_ID::Message() {
  AccountLoginRes* const _this = this; (void)_this;
  new (&_impl_) Impl_{
      decltype(_impl_.account_){}
    , decltype(_impl_.result_){}
    , /*decltype(_impl_._cached_size_)*/{}};

  _internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
  ::memcpy(&_impl_.account_, &from._impl_.account_,
    static_cast<size_t>(reinterpret_cast<char*>(&_impl_.result_) -
    reinterpret_cast<char*>(&_impl_.account_)) + sizeof(_impl_.result_));
  // @@protoc_insertion_point(copy_constructor:cspkg.AccountLoginRes)
}

inline void AccountLoginRes::SharedCtor(
    ::_pb::Arena* arena, bool is_message_owned) {
  (void)arena;
  (void)is_message_owned;
  new (&_impl_) Impl_{
      decltype(_impl_.account_){0u}
    , decltype(_impl_.result_){0}
    , /*decltype(_impl_._cached_size_)*/{}
  };
}

AccountLoginRes::~AccountLoginRes() {
  // @@protoc_insertion_point(destructor:cspkg.AccountLoginRes)
  if (auto *arena = _internal_metadata_.DeleteReturnArena<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>()) {
  (void)arena;
    return;
  }
  SharedDtor();
}

inline void AccountLoginRes::SharedDtor() {
  GOOGLE_DCHECK(GetArenaForAllocation() == nullptr);
}

void AccountLoginRes::SetCachedSize(int size) const {
  _impl_._cached_size_.Set(size);
}

void AccountLoginRes::Clear() {
// @@protoc_insertion_point(message_clear_start:cspkg.AccountLoginRes)
  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  ::memset(&_impl_.account_, 0, static_cast<size_t>(
      reinterpret_cast<char*>(&_impl_.result_) -
      reinterpret_cast<char*>(&_impl_.account_)) + sizeof(_impl_.result_));
  _internal_metadata_.Clear<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
}

const char* AccountLoginRes::_InternalParse(const char* ptr, ::_pbi::ParseContext* ctx) {
#define CHK_(x) if (PROTOBUF_PREDICT_FALSE(!(x))) goto failure
  while (!ctx->Done(&ptr)) {
    uint32_t tag;
    ptr = ::_pbi::ReadTag(ptr, &tag);
    switch (tag >> 3) {
      // fixed32 account = 1;
      case 1:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 13)) {
          _impl_.account_ = ::PROTOBUF_NAMESPACE_ID::internal::UnalignedLoad<uint32_t>(ptr);
          ptr += sizeof(uint32_t);
        } else
          goto handle_unusual;
        continue;
      // int32 result = 2;
      case 2:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 16)) {
          _impl_.result_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint32(&ptr);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      default:
        goto handle_unusual;
    }  // switch
  handle_unusual:
    if ((tag == 0) || ((tag & 7) == 4)) {
      CHK_(ptr);
      ctx->SetLastTag(tag);
      goto message_done;
    }
    ptr = UnknownFieldParse(
        tag,
        _internal_metadata_.mutable_unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(),
        ptr, ctx);
    CHK_(ptr != nullptr);
  }  // while
message_done:
  return ptr;
failure:
  ptr = nullptr;
  goto message_done;
#undef CHK_
}

uint8_t* AccountLoginRes::_InternalSerialize(
    uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const {
  // @@protoc_insertion_point(serialize_to_array_start:cspkg.AccountLoginRes)
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  // fixed32 account = 1;
  if (this->_internal_account() != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteFixed32ToArray(1, this->_internal_account(), target);
  }

  // int32 result = 2;
  if (this->_internal_result() != 0) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteInt32ToArray(2, this->_internal_result(), target);
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    target = ::_pbi::WireFormat::InternalSerializeUnknownFieldsToArray(
        _internal_metadata_.unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(::PROTOBUF_NAMESPACE_ID::UnknownFieldSet::default_instance), target, stream);
  }
  // @@protoc_insertion_point(serialize_to_array_end:cspkg.AccountLoginRes)
  return target;
}

size_t AccountLoginRes::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:cspkg.AccountLoginRes)
  size_t total_size = 0;

  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  // fixed32 account = 1;
  if (this->_internal_account() != 0) {
    total_size += 1 + 4;
  }

  // int32 result = 2;
  if (this->_internal_result() != 0) {
    total_size += ::_pbi::WireFormatLite::Int32SizePlusOne(this->_internal_result());
  }

  return MaybeComputeUnknownFieldsSize(total_size, &_impl_._cached_size_);
}

const ::PROTOBUF_NAMESPACE_ID::Message::ClassData AccountLoginRes::_class_data_ = {
    ::PROTOBUF_NAMESPACE_ID::Message::CopyWithSourceCheck,
    AccountLoginRes::MergeImpl
};
const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*AccountLoginRes::GetClassData() const { return &_class_data_; }


void AccountLoginRes::MergeImpl(::PROTOBUF_NAMESPACE_ID::Message& to_msg, const ::PROTOBUF_NAMESPACE_ID::Message& from_msg) {
  auto* const _this = static_cast<AccountLoginRes*>(&to_msg);
  auto& from = static_cast<const AccountLoginRes&>(from_msg);
  // @@protoc_insertion_point(class_specific_merge_from_start:cspkg.AccountLoginRes)
  GOOGLE_DCHECK_NE(&from, _this);
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  if (from._internal_account() != 0) {
    _this->_internal_set_account(from._internal_account());
  }
  if (from._internal_result() != 0) {
    _this->_internal_set_result(from._internal_result());
  }
  _this->_internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
}

void AccountLoginRes::CopyFrom(const AccountLoginRes& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:cspkg.AccountLoginRes)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool AccountLoginRes::IsInitialized() const {
  return true;
}

void AccountLoginRes::InternalSwap(AccountLoginRes* other) {
  using std::swap;
  _internal_metadata_.InternalSwap(&other->_internal_metadata_);
  ::PROTOBUF_NAMESPACE_ID::internal::memswap<
      PROTOBUF_FIELD_OFFSET(AccountLoginRes, _impl_.result_)
      + sizeof(AccountLoginRes::_impl_.result_)
      - PROTOBUF_FIELD_OFFSET(AccountLoginRes, _impl_.account_)>(
          reinterpret_cast<char*>(&_impl_.account_),
          reinterpret_cast<char*>(&other->_impl_.account_));
}

::PROTOBUF_NAMESPACE_ID::Metadata AccountLoginRes::GetMetadata() const {
  return ::_pbi::AssignDescriptors(
      &descriptor_table_cs_5fproto_2frole_2eproto_getter, &descriptor_table_cs_5fproto_2frole_2eproto_once,
      file_level_metadata_cs_5fproto_2frole_2eproto[1]);
}

// @@protoc_insertion_point(namespace_scope)
}  // namespace cspkg
PROTOBUF_NAMESPACE_OPEN
template<> PROTOBUF_NOINLINE ::cspkg::AccountLoginReq*
Arena::CreateMaybeMessage< ::cspkg::AccountLoginReq >(Arena* arena) {
  return Arena::CreateMessageInternal< ::cspkg::AccountLoginReq >(arena);
}
template<> PROTOBUF_NOINLINE ::cspkg::AccountLoginRes*
Arena::CreateMaybeMessage< ::cspkg::AccountLoginRes >(Arena* arena) {
  return Arena::CreateMessageInternal< ::cspkg::AccountLoginRes >(arena);
}
PROTOBUF_NAMESPACE_CLOSE

// @@protoc_insertion_point(global_scope)
#include <google/protobuf/port_undef.inc>
