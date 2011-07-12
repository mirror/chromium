// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/webui_login_display.h"

#include "chrome/browser/chromeos/login/webui_login_view.h"
#include "chrome/browser/chromeos/wm_ipc.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser_window.h"
#include "grit/chromium_strings.h"
#include "grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "views/widget/widget.h"

#if defined(TOUCH_UI)
#include "chrome/browser/chromeos/login/touch_login_view.h"
#endif

namespace chromeos {

// WebUILoginDisplay, public: --------------------------------------------------

WebUILoginDisplay::~WebUILoginDisplay() {
}

// WebUILoginDisplay, Singleton implementation: --------------------------------

// static
WebUILoginDisplay* WebUILoginDisplay::GetInstance() {
  return Singleton<WebUILoginDisplay>::get();
}

// LoginDisplay implementation: ------------------------------------------------

// static
views::Widget* WebUILoginDisplay::GetLoginWindow() {
  return WebUILoginDisplay::GetInstance()->LoginWindow();
}

views::Widget* WebUILoginDisplay::LoginWindow() {
  return login_window_;
}

void WebUILoginDisplay::Destroy() {
  background_bounds_ = gfx::Rect();
  delegate_ = NULL;
}

  // TODO(rharrison): Add mechanism to pass in the show_guest and show_new_user
  // values.
void WebUILoginDisplay::Init(const std::vector<UserManager::User>& users,
                             bool show_guest,
                             bool show_new_user) {
  // Testing that the delegate has been set.
  DCHECK(delegate_);
  users_ = users;
}

void WebUILoginDisplay::OnBeforeUserRemoved(const std::string& username) {
  // TODO(rharrison): Figure out if I need to split anything between this and
  // OnUserRemoved
}

void WebUILoginDisplay::OnUserImageChanged(UserManager::User* user) {
  // TODO(rharrison): Update the user in the user vector
  // TODO(rharrison): Push the change to WebUI Login screen
}

void WebUILoginDisplay::OnUserRemoved(const std::string& username) {
  // TODO(rharrison): Remove the user from the user vector
  // TODO(rharrison): Push the change to WebUI Login screen
}

void WebUILoginDisplay::OnFadeOut() { }

void WebUILoginDisplay::SetUIEnabled(bool is_enabled) {
  // Send message to WM to enable/disable click on windows.
  WmIpc::Message message(WM_IPC_MESSAGE_WM_SET_LOGIN_STATE);
  message.set_param(0, is_enabled);
  WmIpc::instance()->SendMessage(message);

  if (is_enabled)
    login_handler_->ClearAndEnablePassword();
}

void WebUILoginDisplay::SelectPod(int index) {
  // TODO(rharrison): Figure out what we should be doing here.
}

void WebUILoginDisplay::ShowError(int error_msg_id,
                                  int login_attempts,
                                  HelpAppLauncher::HelpTopic help_topic_id) {
  DCHECK(login_handler_);

  std::string error_text;
  switch (error_msg_id) {
    case IDS_LOGIN_ERROR_AUTHENTICATING_HOSTED:
      error_text = l10n_util::GetStringFUTF8(
          error_msg_id, l10n_util::GetStringUTF16(IDS_PRODUCT_OS_NAME));
      break;
    case IDS_LOGIN_ERROR_CAPTIVE_PORTAL:
      error_text = l10n_util::GetStringFUTF8(
          error_msg_id, delegate()->GetConnectedNetworkName());
      break;
    default:
      error_text = l10n_util::GetStringUTF8(error_msg_id);
      break;
  }

  std::string help_link;
  switch (error_msg_id) {
    case IDS_LOGIN_ERROR_CAPTIVE_PORTAL:
      help_link = l10n_util::GetStringUTF8(IDS_LOGIN_FIX_CAPTIVE_PORTAL);
      break;
    case IDS_LOGIN_ERROR_CAPTIVE_PORTAL_NO_GUEST_MODE:
      // No help link is needed.
      break;
    case IDS_LOGIN_ERROR_AUTHENTICATING_HOSTED:
      help_link = l10n_util::GetStringUTF8(IDS_LEARN_MORE);
      break;
    default:
      if (login_attempts > 1)
        help_link = l10n_util::GetStringUTF8(IDS_LEARN_MORE);
      break;
  }

  login_handler_->ShowError(error_text, help_link, help_topic_id);
}

// WebUILoginDisplay, LoginUIHandlerDelegate implementation: -------------------

void WebUILoginDisplay::Login(const std::string& username,
                              const std::string& password) {
  DCHECK(delegate_);
  delegate_->Login(username, password);
}

void WebUILoginDisplay::LoginAsGuest() {
  DCHECK(delegate_);
  delegate_->LoginAsGuest();
}

// WebUILoginDisplay, private: -------------------------------------------------

// Singleton implementation: ---------------------------------------------------

WebUILoginDisplay::WebUILoginDisplay()
    : LoginDisplay(NULL, gfx::Rect()),
      LoginUIHandlerDelegate(),
      login_window_(NULL) {
}

}  // namespace chromeos
