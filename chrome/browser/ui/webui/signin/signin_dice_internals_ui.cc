// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/signin/signin_dice_internals_ui.h"

#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/browser/ui/webui/signin/signin_dice_internals_handler.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/browser_resources.h"
#include "components/grit/components_resources.h"
#include "components/signin/core/browser/signin_manager.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "ui/base/webui/web_ui_util.h"

SigninDiceInternalsUI::SigninDiceInternalsUI(content::WebUI* web_ui)
    : WebUIController(web_ui), profile_(Profile::FromWebUI(web_ui)) {
  DCHECK(!profile_->IsOffTheRecord());

  SigninManagerFactory::GetForProfile(profile_)->AddObserver(this);

  content::WebUIDataSource* source = content::WebUIDataSource::Create(
      chrome::kChromeUISigninDiceInternalsHost);
  source->SetJsonPath("strings.js");
  source->SetDefaultResource(IDR_SIGNIN_DICE_INTERNALS_HTML);
  source->AddResourcePath("signin_dice_internals.js",
                          IDR_SIGNIN_DICE_INTERNALS_JS);

  source->AddBoolean(
      "isSignedIn",
      SigninManagerFactory::GetForProfile(profile_)->IsAuthenticated());
  source->AddBoolean("isSyncAllowed", profile_->IsSyncAllowed());

  base::DictionaryValue strings;
  webui::SetLoadTimeDataDefaults(g_browser_process->GetApplicationLocale(),
                                 &strings);
  source->AddLocalizedStrings(strings);

  content::WebUIDataSource::Add(profile_, source);
  web_ui->AddMessageHandler(
      base::MakeUnique<SigninDiceInternalsHandler>(profile_));
}

void SigninDiceInternalsUI::GoogleSigninSucceeded(const std::string& account_id,
                                                  const std::string& username) {
  std::unique_ptr<base::DictionaryValue> update(new base::DictionaryValue);
  update->SetBoolean(
      "isSignedIn",
      SigninManagerFactory::GetForProfile(profile_)->IsAuthenticated());
  update->SetBoolean("isSyncAllowed", profile_->IsSyncAllowed());
  content::WebUIDataSource::Update(
      profile_, chrome::kChromeUISigninDiceInternalsHost, std::move(update));

  // Reload the page after the data source was updated.
  web_ui()->CallJavascriptFunctionUnsafe("window.location.reload");
}

void SigninDiceInternalsUI::GoogleSignedOut(const std::string& account_id,
                                            const std::string& username) {
  std::unique_ptr<base::DictionaryValue> update(new base::DictionaryValue);
  update->SetBoolean(
      "isSignedIn",
      SigninManagerFactory::GetForProfile(profile_)->IsAuthenticated());
  update->SetBoolean("isSyncAllowed", profile_->IsSyncAllowed());
  content::WebUIDataSource::Update(
      profile_, chrome::kChromeUISigninDiceInternalsHost, std::move(update));

  // Reload the page after the data source was updated.
  web_ui()->CallJavascriptFunctionUnsafe("window.location.reload");
}

SigninDiceInternalsUI::~SigninDiceInternalsUI() {
  SigninManagerFactory::GetForProfile(profile_)->RemoveObserver(this);
}
