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
    // Total frame latency of all commit.
    base::TimeDelta frame_latency;

    // Total presentation latency of all presented frames.
    base::TimeDelta presentation_latency;

    // Total presentation ipc latency of all presented frames.
    base::TimeDelta presentation_ipc_latency;

    // Number of frames received successfully.
    uint32_t num_frames = 0;

    // Number of presented frames.
    uint32_t num_frames_presented = 0;

    // Number of discarded frames.
    uint32_t num_frames_discarded = 0;
  };
  void Run(int frames, Result* result = nullptr);

 private:
  DISALLOW_COPY_AND_ASSIGN(Simple);
};

}  // namespace clients
}  // namespace wayland
}  // namespace exo

#endif  // COMPONENTS_EXO_WAYLAND_CLIENTS_SIMPLE_H_
