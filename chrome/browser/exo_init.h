// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXO_INIT_H_
#define CHROME_BROWSER_EXO_INIT_H_

#include <memory>

#include "base/macros.h"

namespace exo {
class Display;
class WMHelper;
namespace wayland {
class Server;
}
}  // namespace exo

namespace arc {
class ArcNotificationSurfaceManagerImpl;
}

class ExoInit {
 public:
  // Creates ExoInit. Returns null if exo should not be created.
  static std::unique_ptr<ExoInit> Create();

  ~ExoInit();

 private:
  ExoInit();

  std::unique_ptr<arc::ArcNotificationSurfaceManagerImpl>
      arc_notification_surface_manager_;
  std::unique_ptr<exo::WMHelper> wm_helper_;
  std::unique_ptr<exo::Display> display_;
  std::unique_ptr<exo::wayland::Server> wayland_server_;
  class WaylandWatcher;
  std::unique_ptr<WaylandWatcher> wayland_watcher_;

  DISALLOW_COPY_AND_ASSIGN(ExoInit);
};

#endif  // CHROME_BROWSER_EXO_INIT_H_
