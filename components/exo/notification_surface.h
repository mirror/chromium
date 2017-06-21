// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_NOTIFICATION_SURFACE_H_
#define COMPONENTS_EXO_NOTIFICATION_SURFACE_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "components/exo/surface_tree_host_delegate.h"
#include "ui/gfx/geometry/size.h"

namespace aura {
class Window;
}

namespace exo {
class NotificationSurfaceManager;
class Surface;
class SurfaceTreeHost;

// Handles notification surface role of a given surface.
class NotificationSurface : public SurfaceTreeHostDelegate {
 public:
  NotificationSurface(NotificationSurfaceManager* manager,
                      Surface* surface,
                      const std::string& notification_key);
  ~NotificationSurface() override;

  const gfx::Size& GetSize() const;

  aura::Window* window() { return window_.get(); }
  const std::string& notification_key() const { return notification_key_; }

  // Overridden from SurfaceTreeHostDelegate:
  void OnSurfaceTreeCommit() override;
  void OnSurfaceDetached(Surface* surface) override;

 private:
  NotificationSurfaceManager* const manager_;  // Not owned.
  std::unique_ptr<SurfaceTreeHost> surface_tree_host_;
  const std::string notification_key_;

  std::unique_ptr<aura::Window> window_;
  bool added_to_manager_ = false;

  DISALLOW_COPY_AND_ASSIGN(NotificationSurface);
};

}  // namespace exo

#endif  // COMPONENTS_EXO_NOTIFICATION_SURFACE_H_
