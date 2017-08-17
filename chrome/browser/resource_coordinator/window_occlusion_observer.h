// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RESOURCE_COORDINATOR_WINDOW_OCCLUSION_OBSERVER_H_
#define CHROME_BROWSER_RESOURCE_COORDINATOR_WINDOW_OCCLUSION_OBSERVER_H_

#include "ui/gfx/native_widget_types.h"

namespace resource_coordinator {

class WindowOcclusionObserver {
 public:
  // Called when the occlusion state of |window| changes. |is_occluded| is
  // true if |window| is completely occluded.
  virtual void OnWindowOcclusionStateChanged(gfx::NativeWindow window,
                                             bool is_occluded) = 0;
};

}  // namespace resource_coordinator

#endif  // CHROME_BROWSER_RESOURCE_COORDINATOR_WINDOW_OCCLUSION_OBSERVER_H_
