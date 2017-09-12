// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_INTERVENTIONS_INTERVENTIONS_UI_H_
#define CHROME_BROWSER_UI_WEBUI_INTERVENTIONS_INTERVENTIONS_UI_H_

#include "base/macros.h"
#include "content/public/browser/web_ui_controller.h"

class InterventionsUI : public content::WebUIController {
 public:
  explicit InterventionsUI(content::WebUI* web_ui);

 private:
  DISALLOW_COPY_AND_ASSIGN(InterventionsUI);
};

#endif  // CHROME_BROWSER_UI_WEBUI_INTERVENTIONS_INTERVENTIONS_UI_H_
