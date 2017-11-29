// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_TEST_WINDOW_TEST_API_H_
#define UI_AURA_TEST_WINDOW_TEST_API_H_

#include "base/macros.h"

namespace aura {

class Window;

namespace test {

class WindowTestApi {
 public:
  explicit WindowTestApi(Window* window);

  bool OwnsLayer() const;

  bool ContainsMouse() const;

  // Sets the occlusion state of the window and notifies the delegate if it is
  // different from the previous state. Note: Any change to this window or
  // another window with the same root can cause occlusion states to be
  // recomputed and the value set by this method to be overridden.
  void SetOccluded(bool occluded);

 private:
  Window* window_;

  DISALLOW_COPY_AND_ASSIGN(WindowTestApi);
};

}  // namespace test
}  // namespace aura

#endif  // UI_AURA_TEST_WINDOW_TEST_API_H_
