// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_WAYLAND_CLIENTS_RECTS_H_
#define COMPONENTS_EXO_WAYLAND_CLIENTS_RECTS_H_

#include "base/macros.h"
#include "components/exo/wayland/clients/client_base.h"

namespace exo {
namespace wayland {
namespace clients {

class Rects : public ClientBase {
 public:
  Rects();

  // Initialize and run client main loop.
  bool Run(size_t frames,
           size_t max_frames_pending,
           size_t num_rects,
           size_t num_benchmark_runs,
           base::TimeDelta benchmark_interval,
           bool show_fps_counter);

 private:
  DISALLOW_COPY_AND_ASSIGN(Rects);
};

}  // namespace clients
}  // namespace wayland
}  // namespace exo

#endif  // COMPONENTS_EXO_WAYLAND_CLIENTS_RECTS_H_
