// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_OOBE_UI_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_OOBE_UI_H_
#pragma once

#include "chrome/browser/chromeos/login/oobe_display.h"
#include "chrome/browser/ui/webui/chrome_web_ui.h"

namespace base {
class DictionaryValue;
}

namespace chromeos {
class SigninScreenHandler;
}

namespace chromeos {

// Base class for the OOBE WebUI handlers.
class OobeMessageHandler : public WebUIMessageHandler {
 public:
  OobeMessageHandler();
  virtual ~OobeMessageHandler();

  // Gets localized strings to be used on the page.
  virtual void GetLocalizedStrings(
      base::DictionaryValue* localized_strings) = 0;

  // This method is called when page is ready. It propagates to inherited class
  // via virtual Initialize() method (see below).
  void InitializeBase();

 protected:
  // Called when the page is ready and handler can do initialization.
  virtual void Initialize() = 0;

  // Whether page is ready.
  bool page_is_ready() const { return page_is_ready_; }

 private:
  // Keeps whether page is ready.
  bool page_is_ready_;

  DISALLOW_COPY_AND_ASSIGN(OobeMessageHandler);
};

// A custom WebUI that defines datasource for out-of-box-experience (OOBE) UI:
// - welcome screen (setup language/keyboard/network).
// - eula screen (CrOS (+ OEM) EULA content/TPM password/crash reporting).
// - update screen.
class OobeUI : public OobeDisplay,
               public ChromeWebUI {
 public:
  explicit OobeUI(TabContents* contents);

  // OobeDisplay implementation:
  virtual void ShowScreen(WizardScreen* screen);
  virtual void HideScreen(WizardScreen* screen);
  virtual UpdateScreenActor* GetUpdateScreenActor();
  virtual NetworkScreenActor* GetNetworkScreenActor();
  virtual EulaScreenActor* GetEulaScreenActor();
  virtual ViewScreenDelegate* GetEnterpriseEnrollmentScreenActor();
  virtual ViewScreenDelegate* GetUserImageScreenActor();
  virtual ViewScreenDelegate* GetRegistrationScreenActor();
  virtual ViewScreenDelegate* GetHTMLPageScreenActor();

  // Collects localized strings from the owned handlers.
  void GetLocalizedStrings(base::DictionaryValue* localized_strings);

  // Initializes the handlers.
  void InitializeHandlers();

  void ShowSigninScreen();

 private:
  void AddOobeMessageHandler(OobeMessageHandler* handler);

  // Screens actors. Note, OobeUI owns them via |handlers_|, not directly here.
  UpdateScreenActor* update_screen_actor_;
  NetworkScreenActor* network_screen_actor_;
  EulaScreenActor* eula_screen_actor_;
  SigninScreenHandler* signin_screen_handler_;

  DISALLOW_COPY_AND_ASSIGN(OobeUI);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_OOBE_UI_H_
