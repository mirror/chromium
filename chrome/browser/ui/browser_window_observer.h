// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_BROWSER_WINDOW_OBSERVER_H_
#define CHROME_BROWSER_UI_BROWSER_WINDOW_OBSERVER_H_

// Observes changes to the browser window.
class BrowserWindowObserver {
 public:
  // Called when the window show state may have changed.
  virtual void OnShowStateChanged() = 0;

 protected:
  virtual ~BrowserWindowObserver() = default;
};

#endif  // CHROME_BROWSER_UI_BROWSER_WINDOW_OBSERVER_H_
