// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/bindings/lib/serialization_context.h"

#include <limits>

#include "base/logging.h"
#include "mojo/public/cpp/system/core.h"

namespace mojo {
namespace internal {

SerializedHandleVector::SerializedHandleVector() = default;

SerializedHandleVector::~SerializedHandleVector() = default;

void SerializedHandleVector::AddHandle(mojo::ScopedHandle handle) {
  DCHECK_LT(handles_.size(), std::numeric_limits<uint32_t>::max());
  if (handle.is_valid()) {
    serialized_handles_.emplace_back(static_cast<uint32_t>(handles_.size()));
    handles_.emplace_back(std::move(handle));
  } else {
    serialized_handles_.emplace_back(kEncodedInvalidHandleValue);
  }
}

void SerializedHandleVector::AddInterface(mojo::ScopedHandle handle,
                                          uint32_t version) {
  AddHandle(std::move(handle));
  interface_versions_.emplace_back(version);
}

void SerializedHandleVector::CopyNextSerializedHandle(Handle_Data* data) {
  DCHECK_LT(next_serialized_handle_to_copy_, serialized_handles_.size());
  data->value = serialized_handles_[next_serialized_handle_to_copy_++];
}

void SerializedHandleVector::CopyNextSerializedInterface(Interface_Data* data) {
  CopyNextSerializedHandle(&data->handle);
  DCHECK_LT(next_interface_version_to_copy_, interface_versions_.size());
  data->version = interface_versions_[next_interface_version_to_copy_++];
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

}  // namespace internal
}  // namespace mojo
