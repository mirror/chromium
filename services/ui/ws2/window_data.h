// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS2_WINDOW_DATA_H_
#define SERVICES_UI_WS2_WINDOW_DATA_H_

#include "base/component_export.h"
#include "base/macros.h"
#include "components/viz/common/surfaces/frame_sink_id.h"
#include "services/ui/ws2/ids.h"
#include "services/viz/public/interfaces/compositing/compositor_frame_sink.mojom.h"

namespace aura {
class Window;
}

namespace ui {
namespace ws2 {

class WindowHostFrameSinkClient;
class WindowServiceClient;

// Tracks any state associated with an aura::Window for the WindowService.
class COMPONENT_EXPORT(WINDOW_SERVICE) WindowData {
 public:
  ~WindowData();

  static WindowData* Create(aura::Window* window,
                            WindowServiceClient* client,
                            WindowId window_id,
                            const viz::FrameSinkId& frame_sink_id);
  static WindowData* Get(aura::Window* window) {
    return const_cast<WindowData*>(
        Get(const_cast<const aura::Window*>(window)));
  }
  static const WindowData* Get(const aura::Window* window);

  WindowServiceClient* owning_window_service_client() {
    return owning_window_service_client_;
  }
  const WindowServiceClient* owning_window_service_client() const {
    return owning_window_service_client_;
  }

  void set_embedded_window_service_client(WindowServiceClient* client) {
    embedded_window_service_client_ = client;
  }
  WindowServiceClient* embedded_window_service_client() {
    return embedded_window_service_client_;
  }
  const WindowServiceClient* embedded_window_service_client() const {
    return embedded_window_service_client_;
  }

  WindowId window_id() const { return window_id_; }
  const viz::FrameSinkId& frame_sink_id() const { return frame_sink_id_; }

  void SetFrameSinkId(const viz::FrameSinkId& frame_sink_id);

  void AttachCompositorFrameSink(
      viz::mojom::CompositorFrameSinkRequest compositor_frame_sink,
      viz::mojom::CompositorFrameSinkClientPtr client);

 private:
  WindowData(aura::Window*,
             WindowServiceClient* client,
             WindowId window_id,
             const viz::FrameSinkId& frame_sink_id);

  void UpdateFrameSinkIdInFrameSinkManager(
      const viz::FrameSinkId& frame_sink_id);

  aura::Window* window_;

  // Client that created the window. Null if the window was created locally,
  // not at the request of another client.
  WindowServiceClient* owning_window_service_client_;

  // This is the WindowServiceClient that has the Window as a root. This is
  // only set at embed points.
  WindowServiceClient* embedded_window_service_client_ = nullptr;

  // This is the id internally assigned. High part is client id, low part is
  // an increasing integer.
  const WindowId window_id_;

  // TODO(sky): wire this up, see ServerWindow::UpdateFrameSinkId(). This is
  // initially |client_window_id|. If the window is used as the embed root, then
  // it changes to high part = id of client being embedded in and low part 0.
  // If used as a top-level, the changed to client_window_id assigned at the
  // time top-level is created by client requesting top-level.
  // XXX make sure I get this right.
  viz::FrameSinkId frame_sink_id_;

  std::unique_ptr<WindowHostFrameSinkClient> window_host_frame_sink_client_;

  DISALLOW_COPY_AND_ASSIGN(WindowData);
};

}  // namespace ws2
}  // namespace ui

#endif  // SERVICES_UI_WS2_WINDOW_DATA_H_
