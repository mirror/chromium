// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chromeos/login/login_ui.h"

#include "base/memory/ref_counted_memory.h"
#include "base/memory/singleton.h"
#include "base/string_piece.h"
#include "base/values.h"
#include "chrome/browser/chromeos/cros/cros_library.h"
#include "chrome/browser/chromeos/cros/power_library.h"
#include "chrome/browser/chromeos/login/webui_login_display.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/webui/chrome_url_data_manager.h"
#include "chrome/browser/ui/webui/chromeos/login/login_ui_helpers.h"
#include "chrome/common/url_constants.h"
#include "content/browser/browser_thread.h"
#include "content/browser/tab_contents/tab_contents.h"

namespace {
const char* kResetPrompt = "resetPrompt";
}  // namespace

namespace chromeos {

// LoginUIHTMLSource, public: --------------------------------------------------

LoginUIHTMLSource::LoginUIHTMLSource(MessageLoop* message_loop)
    : DataSource(chrome::kChromeUILoginHost, message_loop),
      html_operations_(new HTMLOperationsInterface()) {
}

LoginUIHTMLSource::~LoginUIHTMLSource() {
}

void LoginUIHTMLSource::StartDataRequest(const std::string& path,
                                         bool is_incognito,
                                         int request_id) {
  DictionaryValue localized_strings;
  SetFontAndTextDirection(&localized_strings);

  base::StringPiece login_html = html_operations_->GetLoginHTML();
  std::string full_html = html_operations_->GetFullHTML(login_html,
                                                        &localized_strings);
  scoped_refptr<RefCountedBytes> html_bytes(
      html_operations_->CreateHTMLBytes(full_html));
  SendResponse(request_id,
               (html_bytes.get()));
}

std::string LoginUIHTMLSource::GetMimeType(const std::string&) const {
  return "text/html";
}

// LoginUIHandlerDelegate, public: ------------------------------------------

void LoginUIHandlerDelegate::set_login_handler(
    BaseLoginUIHandler* login_handler) {
  login_handler_ = login_handler;
}

// LoginUIHandlerDelegate, protected: ------------------------------------------

LoginUIHandlerDelegate::~LoginUIHandlerDelegate() {}

// LoginUIHandler, public: -----------------------------------------------------

LoginUIHandler::LoginUIHandler() {
  delegate_ = WebUILoginDisplay::GetInstance();
  delegate_->set_login_handler(this);
}

LoginUIHandler::~LoginUIHandler() {
}

WebUIMessageHandler* LoginUIHandler::Attach(WebUI* web_ui) {
  return WebUIMessageHandler::Attach(web_ui);
}

void LoginUIHandler::RegisterMessages() {
  web_ui_->RegisterMessageCallback(
      "LaunchIncognito",
      NewCallback(this,
                  &LoginUIHandler::HandleLaunchIncognito));
  web_ui_->RegisterMessageCallback(
      "AuthenticateUser",
      NewCallback(this,
                  &LoginUIHandler::HandleAuthenticateUser));
  web_ui_->RegisterMessageCallback(
      "ShutdownSystem",
      NewCallback(this,
                  &LoginUIHandler::HandleShutdownSystem));
}

// LoginUIHandler, private: ----------------------------------------------------

void LoginUIHandler::ClearAndEnablePassword() {
  web_ui_->CallJavascriptFunction(kResetPrompt);
}

void LoginUIHandler::ShowError(const std::string& error_text,
                               const std::string& help_link_text,
                               HelpAppLauncher::HelpTopic help_topic_id) {
  // TODO(xiyuan): Pass error + help to a propery error UI and save topic id.
  ClearAndEnablePassword();
}

void LoginUIHandler::HandleAuthenticateUser(const ListValue* args) {
  std::string username;
  std::string password;
  if (!args->GetString(0, &username) ||
      !args->GetString(1, &password)) {
    NOTREACHED();
    return;
  }

  delegate_->Login(username, password);
}

void LoginUIHandler::HandleLaunchIncognito(const ListValue* args) {
  delegate_->LoginAsGuest();
}

void LoginUIHandler::HandleShutdownSystem(const ListValue* args) {
  DCHECK(CrosLibrary::Get()->EnsureLoaded());
  CrosLibrary::Get()->GetPowerLibrary()->RequestShutdown();
}

// LoginUI, public: ------------------------------------------------------------

LoginUI::LoginUI(TabContents* contents)
    : ChromeWebUI(contents) {
  LoginUIHandler* handler = new LoginUIHandler();
  AddMessageHandler(handler->Attach(this));
  LoginUIHTMLSource* html_source =
      new LoginUIHTMLSource(MessageLoop::current());

  contents->profile()->GetChromeURLDataManager()->AddDataSource(html_source);
}

}  // namespace chromeos
