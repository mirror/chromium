// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_WAYLAND_CLIENTS_SIMPLE_H_
#define COMPONENTS_EXO_WAYLAND_CLIENTS_SIMPLE_H_

#include "base/time/time.h"
#include "components/exo/wayland/clients/client_base.h"

namespace exo {
namespace wayland {
namespace clients {

class Simple : public wayland::clients::ClientBase {
 public:
  Simple();
  struct Result {
    // Latency between wl_surface_commit and the frame callback.
    base::TimeDelta frame_latency;

    // latency between wl_surface_commit and surface being presented on screen.
    base::TimeDelta presentation_latency;

    // latency between surface being presented on screen and the presentation
    // feedback being received by client.
    base::TimeDelta presentation_ipc_latency;
  };
  void Run(int frames, Result* result = nullptr);

 private:
  DISALLOW_COPY_AND_ASSIGN(Simple);
};

}  // namespace clients
}  // namespace wayland
}  // namespace exo

#endif  // COMPONENTS_EXO_WAYLAND_CLIENTS_SIMPLE_H_
