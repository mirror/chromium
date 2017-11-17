// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/gpu_fence.h"

namespace gfx {

GpuFence::GpuFence() = default;

GpuFence::~GpuFence() = default;

ClientGpuFence GpuFence::AsClientGpuFence() {
  return reinterpret_cast<ClientGpuFence>(this);
}

// static
GpuFence* GpuFence::FromClientGpuFence(ClientGpuFence gpu_fence) {
  return reinterpret_cast<GpuFence*>(gpu_fence);
}

}  // namespace gfx
