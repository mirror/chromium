// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_BROWSER_WINDOW_OBSERVER_H_
#define CHROME_BROWSER_UI_BROWSER_WINDOW_OBSERVER_H_

#include "ui/base/ui_base_types.h"

// Observes changes to window activation and show state.
class BrowserWindowObserver {
 public:
  // Calls when the window bounds or show state change.
  virtual void OnShowStateChanged(ui::WindowShowState show_state) {}

 protected:
  virtual ~BrowserWindowObserver() = default;
};

#endif  // CHROME_BROWSER_UI_BROWSER_WINDOW_OBSERVER_H_
