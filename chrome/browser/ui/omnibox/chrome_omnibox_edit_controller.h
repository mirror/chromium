// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_OMNIBOX_CHROME_OMNIBOX_EDIT_CONTROLLER_H_
#define CHROME_BROWSER_UI_OMNIBOX_CHROME_OMNIBOX_EDIT_CONTROLLER_H_

#include "base/macros.h"
#include "components/omnibox/browser/omnibox_edit_controller.h"

class CommandUpdaterProxy;

namespace content {
class WebContents;
}

// Chrome-specific extension of the OmniboxEditController base class.
class ChromeOmniboxEditController : public OmniboxEditController {
 public:
  // OmniboxEditController:
  void OnAutocompleteAccept(const GURL& destination_url,
                            WindowOpenDisposition disposition,
                            ui::PageTransition transition,
                            AutocompleteMatchType::Type type) override;
  void OnInputInProgress(bool in_progress) override;
  bool SwitchToTabWithURL(const std::string& url, bool close_this) override;

  // Returns the WebContents of the currently active tab.
  virtual content::WebContents* GetWebContents() = 0;

  // Called when the the controller should update itself without restoring any
  // tab state.
  virtual void UpdateWithoutTabRestore() = 0;

  CommandUpdaterProxy* command_updater_proxy() {
    return command_updater_proxy_;
  }
  const CommandUpdaterProxy* command_updater_proxy() const {
    return command_updater_proxy_;
  }

 protected:
  explicit ChromeOmniboxEditController(
      CommandUpdaterProxy* command_updater_proxy);
  ~ChromeOmniboxEditController() override;

 private:
  CommandUpdaterProxy* const command_updater_proxy_;

  DISALLOW_COPY_AND_ASSIGN(ChromeOmniboxEditController);
};

#endif  // CHROME_BROWSER_UI_OMNIBOX_CHROME_OMNIBOX_EDIT_CONTROLLER_H_
