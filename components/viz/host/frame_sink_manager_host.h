// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_HOST_FRAME_SINK_MANAGER_HOST_H_
#define COMPONENTS_VIZ_HOST_FRAME_SINK_MANAGER_HOST_H_

#include <unordered_map>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "cc/ipc/frame_sink_manager.mojom.h"
#include "cc/surfaces/frame_sink_id.h"
#include "components/viz/frame_sinks/mojo_frame_sink_manager.h"
#include "components/viz/host/frame_sink_observer.h"
#include "components/viz/host/viz_host_export.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace cc {
class SurfaceInfo;
class SurfaceManager;
}

namespace viz {

// Browser side implementation of mojom::FrameSinkManager. Manages frame sinks
// and is intended to replace SurfaceManager.
class VIZ_HOST_EXPORT FrameSinkManagerHost
    : NON_EXPORTED_BASE(cc::mojom::FrameSinkManagerClient) {
 public:
  FrameSinkManagerHost();
  ~FrameSinkManagerHost() override;

  cc::SurfaceManager* surface_manager();

  // Connects to MojoFrameSinkManager asnychronously via Mojo.
  void ConnectToFrameSinkManager();

  // Connects to MojoFrameSinkManager synchronously for tests.
  void ConnectForTest();

  void AddObserver(FrameSinkObserver* observer);
  void RemoveObserver(FrameSinkObserver* observer);

  // See frame_sink_manager.mojom for descriptions.
  void CreateCompositorFrameSink(
      const cc::FrameSinkId& frame_sink_id,
      cc::mojom::MojoCompositorFrameSinkRequest request,
      cc::mojom::MojoCompositorFrameSinkPrivateRequest private_request,
      cc::mojom::MojoCompositorFrameSinkClientPtr client);
  void RegisterFrameSinkHierarchy(const cc::FrameSinkId& parent_frame_sink_id,
                                  const cc::FrameSinkId& child_frame_sink_id);
  void UnregisterFrameSinkHierarchy(const cc::FrameSinkId& parent_frame_sink_id,
                                    const cc::FrameSinkId& child_frame_sink_id);

 private:
  // cc::mojom::FrameSinkManagerClient:
  void OnSurfaceCreated(const cc::SurfaceInfo& surface_info) override;

  // This will point at |frame_sink_manager_mojo_| if there is a Mojo connection
  // and calls will happen asynchronously. For tests this can point directly at
  // |frame_sink_manager_| and requests will happen synchronously.
  cc::mojom::FrameSinkManager* frame_sink_manager_ptr_ = nullptr;

  // Mojo connection to |frame_sink_manager_|.
  cc::mojom::FrameSinkManagerPtr frame_sink_manager_mojo_;

  // Mojo connection back from |frame_sink_manager_|.
  mojo::Binding<cc::mojom::FrameSinkManagerClient> binding_;

  // This is owned here so that SurfaceManager will be accessible in process.
  // Other than using SurfaceManager, access to |frame_sink_manager_| should
  // happen using Mojo. See http://crbug.com/657959.
  MojoFrameSinkManager frame_sink_manager_;

  // Directed graph of FrameSinkId hierarchy where key is the child and value is
  // the parent. This hiearchy is used to find the parent that is expected to
  // embed another FrameSink.
  std::unordered_map<cc::FrameSinkId, cc::FrameSinkId, cc::FrameSinkIdHash>
      frame_sink_hiearchy_;

  // Local observers to that receive OnSurfaceCreated() messages from IPC.
  base::ObserverList<FrameSinkObserver> observers_;

  DISALLOW_COPY_AND_ASSIGN(FrameSinkManagerHost);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_HOST_FRAME_SINK_MANAGER_HOST_H_
