// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cronet/native/buffer.h"

#include "base/logging.h"
#include "base/macros.h"

namespace {

// TODO(mef): Make it a singleton, so it would not have to be allocated for
// every buffer.
void Cronet_BufferCallback_OnDestoy_malloc(Cronet_BufferCallbackPtr self,
                                           Cronet_BufferPtr buffer) {
  free(buffer->GetData());
  Cronet_BufferCallback_Destroy(self);
}

}  // namespace

namespace cronet {

Cronet_BufferImpl::Cronet_BufferImpl() = default;
Cronet_BufferImpl::~Cronet_BufferImpl() {
  if (callback_)
    callback_->OnDestroy(this);
}

void Cronet_BufferImpl::SetContext(Cronet_BufferContext context) {
  context_ = context;
}

Cronet_BufferContext Cronet_BufferImpl::GetContext() {
  return context_;
}

void Cronet_BufferImpl::InitWithDataAndCallback(
    RawDataPtr data,
    uint64_t size,
    Cronet_BufferCallbackPtr callback) {
  data_ = data;
  size_ = size;
  callback_ = callback;
}

void Cronet_BufferImpl::InitWithAlloc(uint64_t size) {
  data_ = malloc(size);
  if (!data_)
    return;
  size_ = size;
  callback_ =
      Cronet_BufferCallback_CreateStub(Cronet_BufferCallback_OnDestoy_malloc);
}

uint64_t Cronet_BufferImpl::GetSize() {
  return size_;
}

RawDataPtr Cronet_BufferImpl::GetData() {
  return data_;
}

};  // namespace cronet

CRONET_EXPORT Cronet_BufferPtr Cronet_Buffer_Create() {
  return new cronet::Cronet_BufferImpl();
}
