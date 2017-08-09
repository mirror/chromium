// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/system/broker.h"

#include <magenta/status.h>
#include <magenta/syscalls.h>
#include <deque>

#include "base/logging.h"
#include "mojo/edk/embedder/embedder_internal.h"
#include "mojo/edk/embedder/platform_handle_utils.h"
#include "mojo/edk/embedder/platform_handle_vector.h"
#include "mojo/edk/embedder/platform_shared_buffer.h"
#include "mojo/edk/system/broker_messages.h"
#include "mojo/edk/system/channel.h"

namespace mojo {
namespace edk {

namespace {

Channel::MessagePtr WaitForBrokerMessage(
    PlatformHandle platform_handle,
    BrokerMessageType expected_type,
    size_t expected_num_handles,
    size_t expected_data_size,
    std::deque<PlatformHandle>* incoming_handles) {
  // Wait for a message on the channel.
  mx_signals_t observed = 0;
  mx_status_t result =
      mx_object_wait_one(platform_handle.as_handle(), MX_CHANNEL_READABLE,
                         MX_TIME_INFINITE, &observed);
  if (result != MX_OK) {
    LOG(ERROR) << "WaitForBrokerMessage(mx_object_wait_one): "
               << mx_status_get_string(result);
    return nullptr;
  }

  // Allocate a message of the correct size, to read into.
  Channel::MessagePtr message(new Channel::Message(
      sizeof(BrokerMessageHeader) + expected_data_size, expected_num_handles));

  // Read the message from the channel, and copy out incoming handles.
  uint32_t bytes_read = 0;
  uint32_t handles_read = 0;
  mx_handle_t handles[MX_CHANNEL_MAX_MSG_HANDLES] = {MX_HANDLE_INVALID};
  result = mx_channel_read(platform_handle.as_handle(), 0,
                           const_cast<void*>(message->data()), handles,
                           message->data_num_bytes(), arraysize(handles),
                           &bytes_read, &handles_read);

  std::deque<PlatformHandle> incoming_platform_handles;
  for (size_t i = 0; i < handles_read; ++i)
    incoming_platform_handles.push_back(PlatformHandle::ForHandle(handles[i]));

  bool error = false;
  if (result != MX_OK) {
    LOG(ERROR) << "WaitForBrokerMessage(mx_channel_read): "
               << mx_status_get_string(result);
    error = true;
  } else if (static_cast<size_t>(bytes_read) != message->data_num_bytes()) {
    LOG(ERROR) << "Invalid node channel message";
    error = true;
  } else if (incoming_platform_handles.size() != expected_num_handles) {
    LOG(ERROR) << "Received " << incoming_platform_handles.size()
               << ", expected " << expected_num_handles;
    error = true;
  }

  if (!error) {
    const BrokerMessageHeader* header =
        reinterpret_cast<const BrokerMessageHeader*>(message->payload());
    if (header->type != expected_type) {
      LOG(ERROR) << "Unexpected message";
      error = true;
    }
  }

  if (error) {
    CloseAllPlatformHandles(&incoming_platform_handles);
  } else {
    if (incoming_handles)
      incoming_handles->swap(incoming_platform_handles);
  }

  if (error)
    return nullptr;
  return message;
}

}  // namespace

Broker::Broker(ScopedPlatformHandle platform_handle)
    : sync_channel_(std::move(platform_handle)) {
  CHECK(sync_channel_.is_valid());

  // Wait for the first message, which should contain a handle.
  std::deque<PlatformHandle> incoming_platform_handles;
  if (WaitForBrokerMessage(sync_channel_.get(), BrokerMessageType::INIT, 1, 0,
                           &incoming_platform_handles)) {
    parent_channel_ = ScopedPlatformHandle(incoming_platform_handles.front());
  }
}

Broker::~Broker() = default;

ScopedPlatformHandle Broker::GetParentPlatformHandle() {
  return std::move(parent_channel_);
}

scoped_refptr<PlatformSharedBuffer> Broker::GetSharedBuffer(size_t num_bytes) {
  base::AutoLock lock(lock_);

  BufferRequestData* buffer_request;
  Channel::MessagePtr out_message = CreateBrokerMessage(
      BrokerMessageType::BUFFER_REQUEST, 0, 0, &buffer_request);
  buffer_request->size = num_bytes;

  mx_status_t result =
      mx_channel_write(sync_channel_.get().as_handle(), 0, out_message->data(),
                       out_message->data_num_bytes(), 0, 0);
  if (result != MX_OK) {
    LOG(ERROR) << "GetSharedBuffer(mx_channel_write): "
               << mx_status_get_string(result);
    return nullptr;
  }

  std::deque<PlatformHandle> incoming_platform_handles;
  Channel::MessagePtr message = WaitForBrokerMessage(
      sync_channel_.get(), BrokerMessageType::BUFFER_RESPONSE, 2,
      sizeof(BufferResponseData), &incoming_platform_handles);
  if (message) {
    const BufferResponseData* data = nullptr;
    if (!GetBrokerMessageData(message.get(), &data))
      return nullptr;
    base::UnguessableToken guid =
        base::UnguessableToken::Deserialize(data->guid_high, data->guid_low);
    ScopedPlatformHandle rw_handle(incoming_platform_handles.front());
    incoming_platform_handles.pop_front();
    ScopedPlatformHandle ro_handle(incoming_platform_handles.front());
    return PlatformSharedBuffer::CreateFromPlatformHandlePair(
        num_bytes, guid, std::move(rw_handle), std::move(ro_handle));
  }

  return nullptr;
}

}  // namespace edk
}  // namespace mojo
