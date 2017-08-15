// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/test_gpu_host.h"

namespace ui {
namespace ws {

TestGpuHost::TestGpuHost()
    : frame_sink_manager_(base::MakeUnique<viz::TestFrameSinkManagerImpl>()),
      frame_sink_manager_binding_(frame_sink_manager_.get()) {}

TestGpuHost::~TestGpuHost() {}

void TestGpuHost::CreateFrameSinkManager(
    viz::mojom::FrameSinkManagerRequest request,
    viz::mojom::FrameSinkManagerClientPtr client) {
  frame_sink_manager_binding_.Bind(std::move(request));
}

}  // namespace ws
}  // namespace ui
