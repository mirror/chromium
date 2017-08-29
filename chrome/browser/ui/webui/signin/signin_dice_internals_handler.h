// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_SIGNIN_SIGNIN_DICE_INTERNALS_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_SIGNIN_SIGNIN_DICE_INTERNALS_HANDLER_H_

#include "base/macros.h"
#include "components/signin/core/browser/signin_manager_base.h"
#include "content/public/browser/web_ui_message_handler.h"

namespace base {
class ListValue;
}
class Profile;

class SigninDiceInternalsHandler : public content::WebUIMessageHandler,
                                   public SigninManagerBase::Observer {
 public:
  explicit SigninDiceInternalsHandler(Profile* profile);
  ~SigninDiceInternalsHandler() override;

  // content::WebUIMessageHandler:
  void RegisterMessages() override;

  // SigninManagerBase::Observer:
  void GoogleSigninSucceeded(const std::string& account_id,
                             const std::string& username) override;
  void GoogleSignedOut(const std::string& account_id,
                       const std::string& username) override;

 private:
  // Handler for web-page initialized loaded events.
  void HandleInitialized(const base::ListValue* args);

  // Handler for enable sync event.
  void HandleEnableSync(const base::ListValue* args);

  // Updates the data source with the current sign-in state and reloads the
  // web-page.
  void UpdateDataSourceAndReload();

  Profile* profile_;

  DISALLOW_COPY_AND_ASSIGN(SigninDiceInternalsHandler);
};

#endif  // CHROME_BROWSER_UI_WEBUI_SIGNIN_SIGNIN_DICE_INTERNALS_HANDLER_H_
