// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_IPC_COMMON_FLUSH_PARAMS_H_
#define GPU_IPC_COMMON_FLUSH_PARAMS_H_

#include <stdint.h>
#include <vector>

#include "gpu/command_buffer/common/sync_token.h"
#include "gpu/gpu_export.h"
#include "ui/latency/latency_info.h"

namespace gpu {

struct FlushParams {
  FlushParams();
  FlushParams(const FlushParams& other);
  ~FlushParams();

  int32_t route_id;
  int32_t put_offset;
  uint32_t flush_id;
  std::vector<ui::LatencyInfo> latency_info;
  std::vector<SyncToken> sync_token_fences;
};

}  // namespace gpu

#endif  // GPU_IPC_COMMON_FLUSH_PARAMS_H_
