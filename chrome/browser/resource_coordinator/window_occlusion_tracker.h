// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_WINDOW_OCCLUSION_TRACKER_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_WINDOW_OCCLUSION_TRACKER_H_

#include <memory>

#include "base/macros.h"
#include "ui/gfx/native_widget_types.h"

namespace resource_coordinator {

class WindowOcclusionObserver;

// Keeps track of window occlusion states.
//
// A window is occluded if is minimized or completely covered by another window
// (exception: a mirrored window is not occluded). There are some scenarios in
// which a window stays in the occluded state despite being visible to the user:
// - When hovering the taskbar on Windows
// - When using Exposé on Mac
// - When using Overview on ChromeOS
// - When using Windows+Tab or Alt+Tab on Windows
class WindowOcclusionTracker {
 public:
  WindowOcclusionTracker();
  ~WindowOcclusionTracker();

  // Registers |observer| to be notified when the occlusion state of |window|
  // changes.
  void AddObserver(gfx::NativeWindow window, WindowOcclusionObserver* observer);

  // Unregisters |observer| to be notified when the occlusion state of
  // |window| changes.
  void RemoveObserver(gfx::NativeWindow window,
                      WindowOcclusionObserver* observer);

 private:
  class Impl;

  // Platform-specific implementation.
  const std::unique_ptr<Impl> impl_;

  DISALLOW_COPY_AND_ASSIGN(WindowOcclusionTracker);
};

}  // namespace resource_coordinator

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_WINDOW_OCCLUSION_TRACKER_H_
