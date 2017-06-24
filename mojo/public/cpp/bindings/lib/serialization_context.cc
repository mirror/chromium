// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/bindings/lib/serialization_context.h"

#include <algorithm>
#include <limits>

#include "base/logging.h"
#include "mojo/public/cpp/bindings/message.h"
#include "mojo/public/cpp/system/core.h"

namespace mojo {
namespace internal {

SerializedHandleVector::SerializedHandleVector() = default;

SerializedHandleVector::~SerializedHandleVector() = default;

void SerializedHandleVector::TrackHandle(mojo::Handle handle) {
  tracked_handles_.emplace_back(handle);
}

Handle_Data SerializedHandleVector::AddHandle(mojo::ScopedHandle handle) {
  Handle_Data data;
  if (!handle.is_valid()) {
    data.value = kEncodedInvalidHandleValue;
  } else {
    DCHECK_LT(handles_.size(), std::numeric_limits<uint32_t>::max());
    DCHECK_GE(tracked_handles_.size(), handles_.size() + 1);
    DCHECK_EQ(tracked_handles_[handles_.size()].value(), handle->value());
    data.value = static_cast<uint32_t>(handles_.size());
    handles_.emplace_back(std::move(handle));
  }
  return data;
}

mojo::ScopedHandle SerializedHandleVector::TakeHandle(
    const Handle_Data& encoded_handle) {
  if (!encoded_handle.is_valid())
    return mojo::ScopedHandle();
  DCHECK_LT(encoded_handle.value, handles_.size());
  return std::move(handles_[encoded_handle.value]);
}

void SerializedHandleVector::Swap(std::vector<mojo::ScopedHandle>* other) {
  handles_.swap(*other);
}

SerializationContext::SerializationContext() {}

SerializationContext::~SerializationContext() {
  DCHECK(!custom_contexts || custom_contexts->empty());
}

void SerializationContext::AttachHandlesToMessage(Message* message) {
  associated_endpoint_handles.swap(
      *message->mutable_associated_endpoint_handles());

  // The attached handles must have already been serialized into |*message|, so
  // this just releases the context's ownership.
  std::vector<mojo::ScopedHandle> serialized_handles;
  handles.Swap(&serialized_handles);
  std::for_each(serialized_handles.begin(), serialized_handles.end(),
                [](ScopedHandle& handle) { ignore_result(handle.release()); });
}

}  // namespace internal
}  // namespace mojo
