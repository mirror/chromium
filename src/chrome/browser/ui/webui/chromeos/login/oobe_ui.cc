// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chromeos/login/oobe_ui.h"

#include <string>

#include "base/logging.h"
#include "base/memory/ref_counted_memory.h"
#include "base/values.h"
#include "chrome/browser/browser_about_handler.h"
#include "chrome/browser/chromeos/login/wizard_controller.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/chrome_url_data_manager.h"
#include "chrome/browser/ui/webui/chromeos/login/eula_screen_handler.h"
#include "chrome/browser/ui/webui/chromeos/login/network_screen_handler.h"
#include "chrome/browser/ui/webui/chromeos/login/signin_screen_handler.h"
#include "chrome/browser/ui/webui/chromeos/login/update_screen_handler.h"
#include "chrome/browser/ui/webui/theme_source.h"
#include "chrome/common/jstemplate_builder.h"
#include "chrome/common/url_constants.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "grit/browser_resources.h"
#include "grit/chromium_strings.h"
#include "grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"

namespace {

// JS API callbacks names.
const char kJsApiScreenStateInitialize[] = "screenStateInitialize";

}  // namespace

namespace chromeos {

class OobeUIHTMLSource : public ChromeURLDataManager::DataSource {
 public:
  explicit OobeUIHTMLSource(DictionaryValue* localized_strings);

  // Called when the network layer has requested a resource underneath
  // the path we registered.
  virtual void StartDataRequest(const std::string& path,
                                bool is_incognito,
                                int request_id);
  virtual std::string GetMimeType(const std::string&) const {
    return "text/html";
  }

 private:
  virtual ~OobeUIHTMLSource() {}

  scoped_ptr<DictionaryValue> localized_strings_;
  DISALLOW_COPY_AND_ASSIGN(OobeUIHTMLSource);
};

// OobeMessageHandler ----------------------------------------------------------
OobeMessageHandler::OobeMessageHandler() : page_is_ready_(false) {
}

OobeMessageHandler::~OobeMessageHandler() {
}

void OobeMessageHandler::InitializeBase() {
  page_is_ready_ = true;
  Initialize();
}

// CoreOobeHandler -------------------------------------------------------------

// The core handler for Javascript messages related to the "oobe" view.
class CoreOobeHandler : public OobeMessageHandler {
 public:
  explicit CoreOobeHandler(OobeUI* oobe_ui);
  virtual ~CoreOobeHandler();

  // OobeMessageHandler implementation:
  virtual void GetLocalizedStrings(DictionaryValue* localized_strings);
  virtual void Initialize();

  // WebUIMessageHandler implementation.
  virtual void RegisterMessages();

 private:
  // Handlers for JS WebUI messages.
  void OnInitialized(const ListValue* args);

  OobeUI* oobe_ui_;

  DISALLOW_COPY_AND_ASSIGN(CoreOobeHandler);
};

// OobeUIHTMLSource -------------------------------------------------------

OobeUIHTMLSource::OobeUIHTMLSource(DictionaryValue* localized_strings)
    : DataSource(chrome::kChromeUIOobeHost, MessageLoop::current()),
      localized_strings_(localized_strings) {
}

void OobeUIHTMLSource::StartDataRequest(const std::string& path,
                                        bool is_incognito,
                                        int request_id) {
  static const base::StringPiece html(
      ResourceBundle::GetSharedInstance().GetRawDataResource(IDR_OOBE_HTML));

  const std::string& full_html = jstemplate_builder::GetI18nTemplateHtml(
      html, localized_strings_.get());

  scoped_refptr<RefCountedBytes> html_bytes(new RefCountedBytes());
  html_bytes->data.resize(full_html.size());
  std::copy(full_html.begin(), full_html.end(), html_bytes->data.begin());

  SendResponse(request_id, html_bytes);
}

// CoreOobeHandler ------------------------------------------------------------

CoreOobeHandler::CoreOobeHandler(OobeUI* oobe_ui)
    : oobe_ui_(oobe_ui) {
}

CoreOobeHandler::~CoreOobeHandler() {
}

void CoreOobeHandler::GetLocalizedStrings(DictionaryValue* localized_strings) {
  localized_strings->SetString("productName",
      l10n_util::GetStringUTF16(IDS_SHORT_PRODUCT_NAME));
}

void CoreOobeHandler::Initialize() {
}

void CoreOobeHandler::RegisterMessages() {
  web_ui_->RegisterMessageCallback(kJsApiScreenStateInitialize,
      NewCallback(this, &CoreOobeHandler::OnInitialized));
}

void CoreOobeHandler::OnInitialized(const ListValue* args) {
  oobe_ui_->InitializeHandlers();
}

// OobeUI ----------------------------------------------------------------------

OobeUI::OobeUI(TabContents* contents)
    : ChromeWebUI(contents),
      update_screen_actor_(NULL),
      network_screen_actor_(NULL),
      eula_screen_actor_(NULL),
      signin_screen_handler_(NULL) {
  AddOobeMessageHandler(new CoreOobeHandler(this));

  NetworkScreenHandler* network_screen_handler = new NetworkScreenHandler;
  network_screen_actor_ = network_screen_handler;
  AddOobeMessageHandler(network_screen_handler);

  EulaScreenHandler* eula_screen_handler = new EulaScreenHandler;
  eula_screen_actor_ = eula_screen_handler;
  AddOobeMessageHandler(eula_screen_handler);

  UpdateScreenHandler* update_screen_handler = new UpdateScreenHandler;
  update_screen_actor_ = update_screen_handler;
  AddOobeMessageHandler(update_screen_handler);

  signin_screen_handler_ = new SigninScreenHandler;
  AddOobeMessageHandler(signin_screen_handler_);

  DictionaryValue* localized_strings = new DictionaryValue;
  GetLocalizedStrings(localized_strings);
  OobeUIHTMLSource* html_source =
      new OobeUIHTMLSource(localized_strings);

  // Set up the chrome://theme/ source, for Chrome logo.
  ThemeSource* theme = new ThemeSource(contents->profile());
  contents->profile()->GetChromeURLDataManager()->AddDataSource(theme);

  // Set up the chrome://terms/ data source, for EULA content.
  InitializeAboutDataSource(chrome::kChromeUITermsHost, contents->profile());

  // Set up the chrome://oobe/ source.
  contents->profile()->GetChromeURLDataManager()->AddDataSource(html_source);
}

void OobeUI::ShowScreen(WizardScreen* screen) {
  screen->Show();
}

void OobeUI::HideScreen(WizardScreen* screen) {
  screen->Hide();
}

UpdateScreenActor* OobeUI::GetUpdateScreenActor() {
  return update_screen_actor_;
}

NetworkScreenActor* OobeUI::GetNetworkScreenActor() {
  return network_screen_actor_;
}

EulaScreenActor* OobeUI::GetEulaScreenActor() {
  return eula_screen_actor_;
}

ViewScreenDelegate* OobeUI::GetEnterpriseEnrollmentScreenActor() {
  NOTIMPLEMENTED();
  return NULL;
}

ViewScreenDelegate* OobeUI::GetUserImageScreenActor() {
  NOTIMPLEMENTED();
  return NULL;
}

ViewScreenDelegate* OobeUI::GetRegistrationScreenActor() {
  NOTIMPLEMENTED();
  return NULL;
}

ViewScreenDelegate* OobeUI::GetHTMLPageScreenActor() {
  NOTIMPLEMENTED();
  return NULL;
}

void OobeUI::GetLocalizedStrings(DictionaryValue* localized_strings) {
  // Note, handlers_[0] is a GenericHandler used by the WebUI.
  for (size_t i = 1; i < handlers_.size(); ++i) {
    static_cast<OobeMessageHandler*>(handlers_[i])->
        GetLocalizedStrings(localized_strings);
  }
  ChromeURLDataManager::DataSource::SetFontAndTextDirection(localized_strings);
}

void OobeUI::AddOobeMessageHandler(OobeMessageHandler* handler) {
  AddMessageHandler(handler->Attach(this));
}

void OobeUI::InitializeHandlers() {
  // Note, handlers_[0] is a GenericHandler used by the WebUI.
  for (size_t i = 1; i < handlers_.size(); ++i) {
    static_cast<OobeMessageHandler*>(handlers_[i])->InitializeBase();
  }
}

void OobeUI::ShowSigninScreen() {
  signin_screen_handler_->Show();
}

}  // namespace chromeos
