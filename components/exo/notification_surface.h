// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_NOTIFICATION_SURFACE_H_
#define COMPONENTS_EXO_NOTIFICATION_SURFACE_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "components/exo/surface_observer.h"
#include "components/exo/surface_tree_host.h"
#include "ui/gfx/geometry/size.h"

namespace exo {
class NotificationSurfaceManager;
class Surface;

// Handles notification surface role of a given surface.
class NotificationSurface : public SurfaceTreeHost, public SurfaceObserver {
 public:
  NotificationSurface(NotificationSurfaceManager* manager,
                      Surface* surface,
                      const std::string& notification_key);
  ~NotificationSurface() override;

  // Get the content size of the |root_surface()|.
  const gfx::Size& GetContentSize() const;

  const std::string& notification_key() const { return notification_key_; }

  // Focus the surface window manually to handle key events for notification.
  // Message center is unactivatable by default, but we make it activatable when
  // user is about to use Direct Reply. In that case, we also need to focus the
  // surface window manually to send events to Android.
  void FocusOnSurfaceWindow();

  // Overridden from SurfaceDelegate:
  void OnSurfaceCommit() override;

  // Overridden from SurfaceObserver:
  void OnSurfaceDestroying(Surface* surface) override;

 private:
  NotificationSurfaceManager* const manager_;  // Not owned.
  const std::string notification_key_;

  bool added_to_manager_ = false;

  DISALLOW_COPY_AND_ASSIGN(NotificationSurface);
};

}  // namespace exo

#endif  // COMPONENTS_EXO_NOTIFICATION_SURFACE_H_
