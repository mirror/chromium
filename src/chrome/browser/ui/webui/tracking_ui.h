// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_TRACKING_UI_H_
#define CHROME_BROWSER_UI_WEBUI_TRACKING_UI_H_
#pragma once

#include "chrome/browser/ui/webui/chrome_web_ui.h"

// The C++ back-end for the chrome://tracking2 webui page.
class TrackingUI : public ChromeWebUI {
 public:
  explicit TrackingUI(TabContents* contents);

 private:
  DISALLOW_COPY_AND_ASSIGN(TrackingUI);
};

#endif  // CHROME_BROWSER_UI_WEBUI_TRACKING_UI_H_
