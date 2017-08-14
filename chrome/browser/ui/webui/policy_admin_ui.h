// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_POLICY_ADMIN_UI_H_
#define CHROME_BROWSER_UI_WEBUI_POLICY_ADMIN_UI_H_

#include "base/macros.h"
#include "content/public/browser/web_ui_controller.h"

namespace content {
class WebUI;
}

// The Web UI controller for the chrome://policy-admin page.
class PolicyAdminUI : public content::WebUIController {
 public:
  explicit PolicyAdminUI(content::WebUI* web_ui);
  ~PolicyAdminUI() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(PolicyAdminUI);
};

#endif  // CHROME_BROWSER_UI_WEBUI_POLICY_ADMIN_UI_H_
