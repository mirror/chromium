// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS2_WINDOW_HOST_FRAME_SINK_DATA_H_
#define SERVICES_UI_WS2_WINDOW_HOST_FRAME_SINK_DATA_H_

#include "base/component_export.h"
#include "base/macros.h"
#include "components/viz/common/surfaces/local_surface_id.h"
#include "components/viz/common/surfaces/parent_local_surface_id_allocator.h"
#include "components/viz/host/host_frame_sink_client.h"

namespace aura {
class ClientSurfaceEmbedder;
class Window;
}  // namespace aura

namespace ui {
namespace ws2 {

class COMPONENT_EXPORT(WINDOW_SERVICE) WindowHostFrameSinkClient
    : public viz::HostFrameSinkClient {
 public:
  explicit WindowHostFrameSinkClient(aura::Window* window);
  ~WindowHostFrameSinkClient() override;

  void OnFrameSinkIdChanged();

  // viz::HostFrameSinkClient:
  void OnFirstSurfaceActivation(const viz::SurfaceInfo& surface_info) override;
  void OnFrameTokenChanged(uint32_t frame_token) override;

 private:
  void UpdateClientSurfaceEmbedder();

  aura::Window* window_;
  std::unique_ptr<aura::ClientSurfaceEmbedder> client_surface_embedder_;
  viz::ParentLocalSurfaceIdAllocator parent_local_surface_id_allocator_;
  viz::LocalSurfaceId local_surface_id_;

  DISALLOW_COPY_AND_ASSIGN(WindowHostFrameSinkClient);
};

}  // namespace ws2
}  // namespace ui

#endif  // SERVICES_UI_WS2_WINDOW_HOST_FRAME_SINK_DATA_H_
