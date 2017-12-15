// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_TOUCH_OBSERVER_H_
#define COMPONENTS_EXO_TOUCH_OBSERVER_H_

namespace exo {
class Touch;

// Observers to the Touch are notified when the Touch destructs.
class TouchObserver {
 public:
  virtual ~TouchObserver() {}

  // Called at the top of the touch's destructor, to give observers a change
  // to remove themselves.
  virtual void OnTouchDestroying(Touch* touch) = 0;
};

}  // namespace exo

#endif  // COMPONENTS_EXO_TOUCH_OBSERVER_H_
