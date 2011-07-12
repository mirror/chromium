// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_WEBUI_LOGIN_DISPLAY_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_WEBUI_LOGIN_DISPLAY_H_
#pragma once

#include <string>
#include <vector>

#include "base/memory/scoped_ptr.h"
#include "base/memory/singleton.h"
#include "chrome/browser/chromeos/login/login_display.h"
#include "chrome/browser/chromeos/login/user_manager.h"
#include "chrome/browser/ui/webui/chromeos/login/login_ui.h"
#include "views/widget/widget.h"

namespace gfx {
class Rect;
}  // namespace gfx

namespace chromeos {
// WebUI-based login UI implementation.
// This class is a Singleton. It allows the LoginDisplayHost and LoginUIHandler
// to access it without having to be coupled with each other. It is created with
// NULL for the delegate and a 0-size rectangle for the background
// bounds. Before use these values should be set to a sane value. When done with
// the object, the ExistingUserController should call Destroy and not free the
// pointer, where as accessing classes should do nothing with the pointer.
//
// Expected order of commands to setup for LoginDisplayHost:
//   WebUILoginDisplay::GetInstance();
//   set_delegate(delegate);
//   set_background_bounds(background_bounds());
//   Init();
//
// Expected order of commands to setup for LoginUIHandler:
//   WebUILoginDisplay::GetInstance();
//   set_login_handler(this);

class WebUILoginDisplay : public LoginDisplay,
                          public LoginUIHandlerDelegate {
 public:
  virtual ~WebUILoginDisplay();

  // Singleton implementation:
  static WebUILoginDisplay* GetInstance();

  // Wrapper used to help in routing keyboard key presses into the login
  // screen. This gets the Login Window widget from the Singleton, so that other
  // classes don't need to know we are a Singleton
  static views::Widget* GetLoginWindow();
  views::Widget* LoginWindow();

  // LoginDisplay implementation:
  virtual void Destroy() OVERRIDE;
  virtual void Init(const std::vector<UserManager::User>& users,
                    bool show_guest,
                    bool show_new_user) OVERRIDE;
  virtual void OnBeforeUserRemoved(const std::string& username) OVERRIDE;
  virtual void OnUserImageChanged(UserManager::User* user) OVERRIDE;
  virtual void OnUserRemoved(const std::string& username) OVERRIDE;
  virtual void OnFadeOut() OVERRIDE;
  virtual void SetUIEnabled(bool is_enabled) OVERRIDE;
  virtual void SelectPod(int index) OVERRIDE;
  virtual void ShowError(int error_msg_id,
                         int login_attempts,
                         HelpAppLauncher::HelpTopic help_topic_id) OVERRIDE;

  // LoginUIHandlerDelegate
  virtual void Login(const std::string& username,
                     const std::string& password) OVERRIDE;
  virtual void LoginAsGuest() OVERRIDE;

  void set_login_window(views::Widget* login_window) {
    login_window_ = login_window;
  }

 private:
  // Singleton implementation:
  friend struct DefaultSingletonTraits<WebUILoginDisplay>;
  WebUILoginDisplay();

  // Set of Users in the systemvisible UserControllers.
  std::vector<UserManager::User> users_;

  // Container of the screen we are displaying
  views::Widget* login_window_;

  DISALLOW_COPY_AND_ASSIGN(WebUILoginDisplay);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_WEBUI_LOGIN_DISPLAY_H_
