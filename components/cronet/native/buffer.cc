// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cronet/native/buffer.h"

#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/macros.h"

namespace {

// Implementation of Cronet_BufferCallback that calls free() to malloc() buffer.
class Cronet_BufferCallbackFree : public Cronet_BufferCallback {
 public:
  Cronet_BufferCallbackFree() = default;
  ~Cronet_BufferCallbackFree() override = default;

  // Singleton instance doesn't allow means to set app-specific context.
  void SetContext(Cronet_BufferCallbackContext context) override {}
  Cronet_BufferCallbackContext GetContext() override { return nullptr; }

  void OnDestroy(Cronet_BufferPtr buffer) override { free(buffer->GetData()); }

 private:
  DISALLOW_COPY_AND_ASSIGN(Cronet_BufferCallbackFree);
};

base::LazyInstance<Cronet_BufferCallbackFree>::Leaky
    g_cronet_buffer_callback_free = LAZY_INSTANCE_INITIALIZER;

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
  callback_ = g_cronet_buffer_callback_free.Pointer();
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
