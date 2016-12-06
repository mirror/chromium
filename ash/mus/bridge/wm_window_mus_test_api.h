// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_MUS_BRIDGE_WM_WINDOW_MUS_TEST_API_H_
#define ASH_MUS_BRIDGE_WM_WINDOW_MUS_TEST_API_H_

#include "ash/mus/bridge/wm_window_mus.h"

namespace ash {
namespace mus {

class WmWindowMusTestApi {
 public:
  // Used by tests to set the default value of
  // |WmWindowMus::default_use_empty_minimum_size_for_testing_|. This is needed
  // as tests don't have a good way to reset the value of
  // |use_empty_minimum_size_for_testing_| before the minimum size is queried.
  class GlobalMinimumSizeLock {
   public:
    GlobalMinimumSizeLock();
    ~GlobalMinimumSizeLock();

   private:
    // Number of instances of GlobalMinimumSizeLock that have been created.
    static int instance_count_;

    DISALLOW_COPY_AND_ASSIGN(GlobalMinimumSizeLock);
  };

  explicit WmWindowMusTestApi(WmWindow* window)
      : WmWindowMusTestApi(WmWindowMus::AsWmWindowMus(window)) {}
  explicit WmWindowMusTestApi(WmWindowMus* window) : window_(window) {}
  ~WmWindowMusTestApi() {}

  void set_use_empty_minimum_size(bool value) {
    window_->use_empty_minimum_size_for_testing_ = true;
  }

 private:
  static void SetDefaultUseEmptyMinimumSizeForTesting(bool value);

  WmWindowMus* window_;

  DISALLOW_COPY_AND_ASSIGN(WmWindowMusTestApi);
};

}  // namespace mus
}  // namespace ash

#endif  // ASH_MUS_BRIDGE_WM_WINDOW_MUS_TEST_API_H_
