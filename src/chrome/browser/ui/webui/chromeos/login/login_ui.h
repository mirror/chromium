// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_LOGIN_UI_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_LOGIN_UI_H_
#pragma once

#include <string>

#include "base/memory/scoped_ptr.h"
#include "chrome/browser/chromeos/login/help_app_launcher.h"
#include "chrome/browser/ui/webui/chrome_url_data_manager.h"
#include "chrome/browser/ui/webui/chrome_web_ui.h"

class Profile;

namespace chromeos {

class HTMLOperationsInterface;

// Boilerplate class that is used to associate the LoginUI code with the URL
// "chrome://login"
class LoginUIHTMLSource : public ChromeURLDataManager::DataSource {
 public:
  explicit LoginUIHTMLSource(MessageLoop* message_loop);
  virtual ~LoginUIHTMLSource();

  virtual void StartDataRequest(const std::string& path,
                                bool is_incognito,
                                int request_id);
  virtual std::string GetMimeType(const std::string&) const;

 private:
  scoped_ptr<HTMLOperationsInterface> html_operations_;

  DISALLOW_COPY_AND_ASSIGN(LoginUIHTMLSource);
};

// An interface between WebUILoginDisplay and LoginUIHandler.
class BaseLoginUIHandler {
 public:
  virtual void ClearAndEnablePassword() = 0;
  virtual void ShowError(const std::string& error_text,
                         const std::string& help_link_text,
                         HelpAppLauncher::HelpTopic help_topic_id) = 0;
};

class LoginUIHandlerDelegate {
 public:
  LoginUIHandlerDelegate()
      : login_handler_(NULL) { }

  // Sign in using |username| and |password| specified.
  // Used for both known and new users.
  virtual void Login(const std::string& username,
                     const std::string& password) = 0;

  // Sign in into Guest session.
  virtual void LoginAsGuest() = 0;

  // Let the delegate know about the handler it is supposed to be using.
  virtual void set_login_handler(BaseLoginUIHandler* login_handler);

 protected:
  // Reference to the WebUI handling layer for the login screen
  BaseLoginUIHandler* login_handler_;

  virtual ~LoginUIHandlerDelegate();
};

// Main LoginUI handling function. It handles the WebUI hooks that are supplied
// for the login page to use for authentication.
class LoginUIHandler : public WebUIMessageHandler,
                       public BaseLoginUIHandler {
 public:
  LoginUIHandler();
  virtual ~LoginUIHandler();

  // WebUIMessageHandler implementation.
  virtual WebUIMessageHandler* Attach(WebUI* web_ui) OVERRIDE;
  virtual void RegisterMessages() OVERRIDE;

 protected:
  LoginUIHandlerDelegate* delegate_;

 private:
  // BaseLoginUIHandler implementation.
  virtual void ClearAndEnablePassword() OVERRIDE;
  virtual void ShowError(const std::string& error_text,
                         const std::string& help_link_text,
                         HelpAppLauncher::HelpTopic help_topic_id) OVERRIDE;

  // Callbacks from javascript.
  void HandleAuthenticateUser(const base::ListValue* args);
  void HandleLaunchIncognito(const base::ListValue* args);
  void HandleShutdownSystem(const base::ListValue* args);

  DISALLOW_COPY_AND_ASSIGN(LoginUIHandler);
};

// Boilerplate class that is used to associate the LoginUI code with the WebUI
// code.
class LoginUI : public ChromeWebUI {
 public:
  explicit LoginUI(TabContents* contents);

  // Return the URL for a given search term.
  static const GURL GetLoginURLWithSearchText(const string16& text);

  static RefCountedMemory* GetFaviconResourceBytes();

 private:
  DISALLOW_COPY_AND_ASSIGN(LoginUI);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_LOGIN_UI_H_
