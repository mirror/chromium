// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_SIGNIN_SIGNIN_DICE_INTERNALS_UI_H_
#define CHROME_BROWSER_UI_WEBUI_SIGNIN_SIGNIN_DICE_INTERNALS_UI_H_

#include "base/macros.h"
#include "components/signin/core/browser/signin_manager_base.h"
#include "content/public/browser/web_ui_controller.h"

class Profile;

// The implementation for the chrome://signin-dice-internals page.
class SigninDiceInternalsUI : public content::WebUIController,
                              public SigninManagerBase::Observer {
 public:
  explicit SigninDiceInternalsUI(content::WebUI* web_ui);
  ~SigninDiceInternalsUI() override;

  // SigninManagerBase::Observer:
  void GoogleSigninSucceeded(const std::string& account_id,
                             const std::string& username) override;
  void GoogleSignedOut(const std::string& account_id,
                       const std::string& username) override;

 private:
  Profile* profile_;

  DISALLOW_COPY_AND_ASSIGN(SigninDiceInternalsUI);
};

#endif  // CHROME_BROWSER_UI_WEBUI_SIGNIN_SIGNIN_DICE_INTERNALS_UI_H_
