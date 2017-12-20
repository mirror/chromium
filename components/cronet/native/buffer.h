// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRONET_NATIVE_BUFFER_H_
#define COMPONENTS_CRONET_NATIVE_BUFFER_H_

#include "base/macros.h"
#include "components/cronet/native/generated/cronet.idl_impl_interface.h"

namespace cronet {

// Concrete implementation of abstract Cronet_Buffer interface.
class Cronet_BufferImpl : public Cronet_Buffer {
 public:
  Cronet_BufferImpl();
  ~Cronet_BufferImpl() override;

  void SetContext(Cronet_BufferContext context) override;
  Cronet_BufferContext GetContext() override;

  void InitWithDataAndCallback(RawDataPtr data,
                               uint64_t size,
                               Cronet_BufferCallbackPtr callback) override;
  void InitWithAlloc(uint64_t size) override;
  uint64_t GetSize() override;
  RawDataPtr GetData() override;

 private:
  RawDataPtr data_ = nullptr;
  uint64_t size_ = 0;
  Cronet_BufferCallbackPtr callback_ = nullptr;
  // Buffer context.
  Cronet_BufferContext context_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(Cronet_BufferImpl);
};

};  // namespace cronet

#endif  // COMPONENTS_CRONET_NATIVE_BUFFER_H_
