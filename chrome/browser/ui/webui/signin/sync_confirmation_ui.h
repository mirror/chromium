// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_SIGNIN_SYNC_CONFIRMATION_UI_H_
#define CHROME_BROWSER_UI_WEBUI_SIGNIN_SYNC_CONFIRMATION_UI_H_

#include <map>
#include <memory>
#include <string>

#include "base/macros.h"
#include "chrome/browser/ui/webui/signin/signin_web_dialog_ui.h"

namespace ui {
class WebUI;
}

// WebUI controller for the sync confirmation dialog.
//
// Note: This controller does not set the WebUI message handler. It is
// the responsability of the caller to pass the correct message handler.
class SyncConfirmationUI : public SigninWebDialogUI {
 public:
  explicit SyncConfirmationUI(content::WebUI* web_ui);
  ~SyncConfirmationUI() override;

  virtual const std::map<std::string, int>& GetResourceNameToGrdIdMap();

  // SigninWebDialogUI:
  void InitializeMessageHandlerWithBrowser(Browser* browser) override;

 private:
  std::map<std::string, int> string_resource_name_to_grd_id_map_;

  DISALLOW_COPY_AND_ASSIGN(SyncConfirmationUI);
};

#endif  // CHROME_BROWSER_UI_WEBUI_SIGNIN_SYNC_CONFIRMATION_UI_H_
