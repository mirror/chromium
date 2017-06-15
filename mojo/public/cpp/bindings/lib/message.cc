// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/bindings/message.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <algorithm>
#include <utility>

#include "base/bind.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_local.h"
#include "mojo/public/cpp/bindings/associated_group_controller.h"
#include "mojo/public/cpp/bindings/lib/array_internal.h"

namespace mojo {

namespace {

base::LazyInstance<base::ThreadLocalPointer<internal::MessageDispatchContext>>::
    DestructorAtExit g_tls_message_dispatch_context = LAZY_INSTANCE_INITIALIZER;

base::LazyInstance<base::ThreadLocalPointer<SyncMessageResponseContext>>::
    DestructorAtExit g_tls_sync_response_context = LAZY_INSTANCE_INITIALIZER;

void DoNotifyBadMessage(Message message, const std::string& error) {
  message.NotifyBadMessage(error);
}

size_t ComputeHeaderSize(uint32_t flags, size_t payload_interface_id_count) {
  if (payload_interface_id_count > 0) {
    // Version 2
    return sizeof(internal::MessageHeaderV2) +
           internal::ArrayDataTraits<uint32_t>::GetStorageSize(
               static_cast<uint32_t>(payload_interface_id_count));
  } else if (flags &
             (Message::kFlagExpectsResponse | Message::kFlagIsResponse)) {
    // Version 1
    return sizeof(internal::MessageHeaderV1);
  } else {
    // Version 0
    return sizeof(internal::MessageHeader);
  }
}

size_t ComputeTotalSize(size_t header_size,
                        size_t payload_size,
                        size_t payload_interface_id_count) {
  if (payload_interface_id_count > 0)
    return internal::Align(header_size + internal::Align(payload_size));
  return internal::Align(header_size + payload_size);
}

template <typename HeaderType>
void AllocateHeaderFromBuffer(internal::Buffer* buffer, HeaderType** header) {
  *header = static_cast<HeaderType*>(buffer->Allocate(sizeof(HeaderType)));
  (*header)->num_bytes = sizeof(HeaderType);
}

// An internal message serialiation context used to build a new serialized
// message when all the necessary metadata is available at construction time.
struct MessageInfo {
  MessageInfo(uint32_t name,
              uint32_t flags,
              size_t payload_size,
              size_t payload_interface_id_count,
              std::vector<ScopedHandle> handles,
              internal::Buffer* payload_buffer)
      : name(name),
        flags(flags),
        payload_size(payload_size),
        payload_interface_id_count(payload_interface_id_count),
        header_size(ComputeHeaderSize(flags, payload_interface_id_count)),
        total_size(ComputeTotalSize(header_size,
                                    payload_size,
                                    payload_interface_id_count)),
        handles(std::move(handles)),
        payload_buffer(payload_buffer) {}
  ~MessageInfo() {}

  const uint32_t name;
  const uint32_t flags;
  const size_t payload_size;
  const size_t payload_interface_id_count;
  const size_t header_size;
  const size_t total_size;
  std::vector<ScopedHandle> handles;
  internal::Buffer* const payload_buffer;
};

void GetSerializedSizeFromMessageInfo(uintptr_t context,
                                      size_t* num_bytes,
                                      size_t* num_handles) {
  auto* info = reinterpret_cast<MessageInfo*>(context);
  *num_bytes = info->total_size;
  *num_handles = info->handles.size();
}

void SerializeHandlesFromMessageInfo(uintptr_t context, MojoHandle* handles) {
  auto* info = reinterpret_cast<MessageInfo*>(context);
  for (size_t i = 0; i < info->handles.size(); ++i)
    handles[i] = info->handles[i].release().value();
}

void SerializePayloadFromMessageInfo(uintptr_t context, void* storage) {
  auto* info = reinterpret_cast<MessageInfo*>(context);
  *info->payload_buffer = internal::Buffer(storage, info->total_size);
  memset(storage, 0, info->total_size);
  if (info->payload_interface_id_count > 0) {
    // Version 2
    internal::MessageHeaderV2* header;
    AllocateHeaderFromBuffer(info->payload_buffer, &header);
    header->version = 2;
    header->name = info->name;
    header->flags = info->flags;
    // The payload immediately follows the header.
    header->payload.Set(header + 1);
  } else if (info->flags &
             (Message::kFlagExpectsResponse | Message::kFlagIsResponse)) {
    // Version 1
    internal::MessageHeaderV1* header;
    AllocateHeaderFromBuffer(info->payload_buffer, &header);
    header->version = 1;
    header->name = info->name;
    header->flags = info->flags;
  } else {
    internal::MessageHeader* header;
    AllocateHeaderFromBuffer(info->payload_buffer, &header);
    header->version = 0;
    header->name = info->name;
    header->flags = info->flags;
  }
}

void DestroyMessageInfo(uintptr_t context) {
  // MessageInfo is always stack-allocated, so there's nothing to do here.
}

const MojoMessageOperationThunks kMessageInfoThunks{
    sizeof(MojoMessageOperationThunks),
    &GetSerializedSizeFromMessageInfo,
    &SerializeHandlesFromMessageInfo,
    &SerializePayloadFromMessageInfo,
    &DestroyMessageInfo,
};

}  // namespace

Message::Message() = default;

Message::Message(Message&& other) = default;

Message::Message(uint32_t name,
                 uint32_t flags,
                 size_t payload_size,
                 size_t payload_interface_id_count,
                 std::vector<ScopedHandle> handles) {
  internal::Buffer buffer;
  MessageInfo info(name, flags, payload_size, payload_interface_id_count,
                   std::move(handles), &buffer);
  ScopedMessageHandle handle;
  MojoResult rv = mojo::CreateMessage(reinterpret_cast<uintptr_t>(&info),
                                      &kMessageInfoThunks, &handle);
  DCHECK_EQ(MOJO_RESULT_OK, rv);
  DCHECK(handle.is_valid());

  rv = MojoSerializeMessage(handle->value());
  DCHECK_EQ(MOJO_RESULT_OK, rv);

  handle_ = std::move(handle);
  data_ = buffer.data();
  data_size_ = buffer.size();
  payload_buffer_ = std::move(buffer);
}

Message::Message(ScopedMessageHandle handle) {
  DCHECK(handle.is_valid());

  // Extract any serialized handles if possible.
  uint32_t num_bytes;
  void* buffer;
  uint32_t num_handles = 0;
  MojoResult rv = MojoGetSerializedMessageContents(
      handle->value(), &buffer, &num_bytes, nullptr, &num_handles,
      MOJO_GET_SERIALIZED_MESSAGE_CONTENTS_FLAG_NONE);
  if (rv == MOJO_RESULT_RESOURCE_EXHAUSTED) {
    handles_.resize(num_handles);
    rv = MojoGetSerializedMessageContents(
        handle->value(), &buffer, &num_bytes,
        reinterpret_cast<MojoHandle*>(handles_.data()), &num_handles,
        MOJO_GET_SERIALIZED_MESSAGE_CONTENTS_FLAG_NONE);
    transferable_ = false;
  }

  if (rv != MOJO_RESULT_OK) {
    // Failed to deserialize handles. Leave the Message uninitialized.
    return;
  }

  handle_ = std::move(handle);
  data_ = buffer;
  data_size_ = num_bytes;
}

Message::~Message() = default;

Message& Message::operator=(Message&& other) = default;

void Message::Reset() {
  handle_.reset();
  handles_.clear();
  associated_endpoint_handles_.clear();
  data_ = nullptr;
  data_size_ = 0;
}

const uint8_t* Message::payload() const {
  if (version() < 2)
    return data() + header()->num_bytes;

  DCHECK(!header_v2()->payload.is_null());
  return static_cast<const uint8_t*>(header_v2()->payload.Get());
}

uint32_t Message::payload_num_bytes() const {
  DCHECK_GE(data_num_bytes(), header()->num_bytes);
  size_t num_bytes;
  if (version() < 2) {
    num_bytes = data_num_bytes() - header()->num_bytes;
  } else {
    auto payload_begin =
        reinterpret_cast<uintptr_t>(header_v2()->payload.Get());
    auto payload_end =
        reinterpret_cast<uintptr_t>(header_v2()->payload_interface_ids.Get());
    if (!payload_end)
      payload_end = reinterpret_cast<uintptr_t>(data() + data_num_bytes());
    DCHECK_GE(payload_end, payload_begin);
    num_bytes = payload_end - payload_begin;
  }
  DCHECK_LE(num_bytes, std::numeric_limits<uint32_t>::max());
  return static_cast<uint32_t>(num_bytes);
}

uint32_t Message::payload_num_interface_ids() const {
  auto* array_pointer =
      version() < 2 ? nullptr : header_v2()->payload_interface_ids.Get();
  return array_pointer ? static_cast<uint32_t>(array_pointer->size()) : 0;
}

const uint32_t* Message::payload_interface_ids() const {
  auto* array_pointer =
      version() < 2 ? nullptr : header_v2()->payload_interface_ids.Get();
  return array_pointer ? array_pointer->storage() : nullptr;
}

uint32_t* Message::mutable_payload_interface_ids() {
  auto* array_pointer =
      version() < 2 ? nullptr : header_v2()->payload_interface_ids.Get();
  return array_pointer ? array_pointer->storage() : nullptr;
}

ScopedMessageHandle Message::TakeMojoMessage() {
  // If there are associated endpoints transferred,
  // SerializeAssociatedEndpointHandles() must be called before this method.
  DCHECK(associated_endpoint_handles_.empty());
  DCHECK(transferable_);
  auto handle = std::move(handle_);
  Reset();
  return handle;
}

void Message::NotifyBadMessage(const std::string& error) {
  DCHECK(handle_.is_valid());
  mojo::NotifyBadMessage(handle_.get(), error);
}

void Message::SerializeAssociatedEndpointHandles(
    AssociatedGroupController* group_controller) {
  if (associated_endpoint_handles_.empty())
    return;

  DCHECK_GE(version(), 2u);
  DCHECK(header_v2()->payload_interface_ids.is_null());
  DCHECK(payload_buffer_.is_valid());

  size_t size = associated_endpoint_handles_.size();
  auto* data = internal::Array_Data<uint32_t>::New(size, &payload_buffer_);
  header_v2()->payload_interface_ids.Set(data);

  for (size_t i = 0; i < size; ++i) {
    ScopedInterfaceEndpointHandle& handle = associated_endpoint_handles_[i];

    DCHECK(handle.pending_association());
    data->storage()[i] =
        group_controller->AssociateInterface(std::move(handle));
  }
  associated_endpoint_handles_.clear();
}

bool Message::DeserializeAssociatedEndpointHandles(
    AssociatedGroupController* group_controller) {
  associated_endpoint_handles_.clear();

  uint32_t num_ids = payload_num_interface_ids();
  if (num_ids == 0)
    return true;

  associated_endpoint_handles_.reserve(num_ids);
  uint32_t* ids = header_v2()->payload_interface_ids.Get()->storage();
  bool result = true;
  for (uint32_t i = 0; i < num_ids; ++i) {
    auto handle = group_controller->CreateLocalEndpointHandle(ids[i]);
    if (IsValidInterfaceId(ids[i]) && !handle.is_valid()) {
      // |ids[i]| itself is valid but handle creation failed. In that case, mark
      // deserialization as failed but continue to deserialize the rest of
      // handles.
      result = false;
    }

    associated_endpoint_handles_.push_back(std::move(handle));
    ids[i] = kInvalidInterfaceId;
  }
  return result;
}

PassThroughFilter::PassThroughFilter() {}

PassThroughFilter::~PassThroughFilter() {}

bool PassThroughFilter::Accept(Message* message) { return true; }

SyncMessageResponseContext::SyncMessageResponseContext()
    : outer_context_(current()) {
  g_tls_sync_response_context.Get().Set(this);
}

SyncMessageResponseContext::~SyncMessageResponseContext() {
  DCHECK_EQ(current(), this);
  g_tls_sync_response_context.Get().Set(outer_context_);
}

// static
SyncMessageResponseContext* SyncMessageResponseContext::current() {
  return g_tls_sync_response_context.Get().Get();
}

void SyncMessageResponseContext::ReportBadMessage(const std::string& error) {
  GetBadMessageCallback().Run(error);
}

const ReportBadMessageCallback&
SyncMessageResponseContext::GetBadMessageCallback() {
  if (bad_message_callback_.is_null()) {
    bad_message_callback_ =
        base::Bind(&DoNotifyBadMessage, base::Passed(&response_));
  }
  return bad_message_callback_;
}

MojoResult ReadMessage(MessagePipeHandle handle, Message* message) {
  ScopedMessageHandle message_handle;
  MojoResult rv =
      ReadMessageNew(handle, &message_handle, MOJO_READ_MESSAGE_FLAG_NONE);
  if (rv != MOJO_RESULT_OK)
    return rv;

  *message = Message(std::move(message_handle));
  return MOJO_RESULT_OK;
}

void ReportBadMessage(const std::string& error) {
  internal::MessageDispatchContext* context =
      internal::MessageDispatchContext::current();
  DCHECK(context);
  context->GetBadMessageCallback().Run(error);
}

ReportBadMessageCallback GetBadMessageCallback() {
  internal::MessageDispatchContext* context =
      internal::MessageDispatchContext::current();
  DCHECK(context);
  return context->GetBadMessageCallback();
}

namespace internal {

MessageHeaderV2::MessageHeaderV2() = default;

MessageDispatchContext::MessageDispatchContext(Message* message)
    : outer_context_(current()), message_(message) {
  g_tls_message_dispatch_context.Get().Set(this);
}

MessageDispatchContext::~MessageDispatchContext() {
  DCHECK_EQ(current(), this);
  g_tls_message_dispatch_context.Get().Set(outer_context_);
}

// static
MessageDispatchContext* MessageDispatchContext::current() {
  return g_tls_message_dispatch_context.Get().Get();
}

const ReportBadMessageCallback&
MessageDispatchContext::GetBadMessageCallback() {
  if (bad_message_callback_.is_null()) {
    bad_message_callback_ =
        base::Bind(&DoNotifyBadMessage, base::Passed(message_));
  }
  return bad_message_callback_;
}

// static
void SyncMessageResponseSetup::SetCurrentSyncResponseMessage(Message* message) {
  SyncMessageResponseContext* context = SyncMessageResponseContext::current();
  if (context)
    context->response_ = std::move(*message);
}

}  // namespace internal

}  // namespace mojo
