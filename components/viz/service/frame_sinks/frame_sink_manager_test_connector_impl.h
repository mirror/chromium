// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_SERVICE_FRAME_SINKS_FRAME_SINK_MANAGER_TEST_CONNECTOR_IMPL_H_
#define COMPONENTS_VIZ_SERVICE_FRAME_SINKS_FRAME_SINK_MANAGER_TEST_CONNECTOR_IMPL_H_

#include "base/macros.h"
#include "components/viz/service/viz_service_export.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"
#include "services/viz/privileged/interfaces/compositing/frame_sink_manager.mojom.h"

namespace viz {
class FrameSinkManagerImpl;

// Connects clients to FrameSinkManagerImpl so that they can observer changes
// during testing. This will be owned by the FrameSinkManagerImpl.
//
// After binding clients must add their observers to receive notifications.
class VIZ_SERVICE_EXPORT FrameSinkManagerTestConnectorImpl
    : public mojom::FrameSinkManagerTestConnector {
 public:
  // Creates a FrameSinkManagerTestConnectorImpl which will be owned by
  // |manager|. The provided |request| is bound.
  static void CreateAndBind(
      FrameSinkManagerImpl* manager,
      mojom::FrameSinkManagerTestConnectorRequest request);

  FrameSinkManagerTestConnectorImpl(
      mojom::FrameSinkManagerTestConnectorRequest request);
  ~FrameSinkManagerTestConnectorImpl() override;

  // Notifies observers of the first time a CompositorFrame with a new SurfaceId
  // is activated for the first time.
  void OnFirstSurfaceActivation(const SurfaceInfo& surface_info);

 private:
  // mojom::FrameSinkManagerTestConnector:
  void AddObserver(mojom::FrameSinkManagerTestObserverPtr observer) override;

  // Allows a test interface to connect to observer frame activations.
  mojo::InterfacePtrSet<mojom::FrameSinkManagerTestObserver> observers_;
  mojo::Binding<mojom::FrameSinkManagerTestConnector> binding_;

  DISALLOW_COPY_AND_ASSIGN(FrameSinkManagerTestConnectorImpl);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_SERVICE_FRAME_SINKS_FRAME_SINK_MANAGER_TEST_CONNECTOR_IMPL_H_
