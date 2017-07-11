// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/settings/on_startup_handler.h"

#include <string>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/logging.h"
#include "chrome/browser/extensions/settings_api_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/settings_utils.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_ui.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/management_policy.h"
#include "extensions/common/extension.h"

namespace settings {

// static
const char OnStartupHandler::kOnStartupNtpExtensionEventName[] =
    "update-ntp-extension";

OnStartupHandler::OnStartupHandler(Profile* profile)
    : extension_registry_observer_(this), profile_(profile) {
  DCHECK(profile);
}
OnStartupHandler::~OnStartupHandler() {}

void OnStartupHandler::RegisterMessages() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  extension_registry_observer_.Add(
      extensions::ExtensionRegistry::Get(profile_));

  web_ui()->RegisterMessageCallback(
      "getNtpExtension", base::Bind(&OnStartupHandler::HandleGetNtpExtension,
                                    base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "validateStartupPage",
      base::Bind(&OnStartupHandler::HandleValidateStartupPage,
                 base::Unretained(this)));
}

void OnStartupHandler::OnExtensionUnloaded(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension,
    extensions::UnloadedExtensionReason reason) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!IsJavascriptAllowed())
    return;

  std::unique_ptr<base::Value> extensionInfo = GetNtpExtension();
  FireWebUIListener(kOnStartupNtpExtensionEventName, *extensionInfo);
}

void OnStartupHandler::OnExtensionReady(
    content::BrowserContext* browser_context,
    const extensions::Extension* extension) {
  if (!IsJavascriptAllowed())
    return;

  std::unique_ptr<base::Value> extensionInfo = GetNtpExtension();
  FireWebUIListener(kOnStartupNtpExtensionEventName, *extensionInfo);
}

std::unique_ptr<base::Value> OnStartupHandler::GetNtpExtension() {
  const extensions::Extension* ntp_extension =
      extensions::GetExtensionOverridingNewTabPage(profile_);
  if (!ntp_extension) {
    std::unique_ptr<base::Value> none(new base::Value);
    return none;
  }

  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue);
  dict->SetString("id", ntp_extension->id());
  dict->SetString("name", ntp_extension->name());
  dict->SetBoolean("canBeDisabled",
                   !extensions::ExtensionSystem::Get(profile_)
                        ->management_policy()
                        ->MustRemainEnabled(ntp_extension, nullptr));
  return dict;
}

void OnStartupHandler::HandleGetNtpExtension(const base::ListValue* args) {
  const base::Value* callback_id;
  CHECK(args->Get(0, &callback_id));
  AllowJavascript();

  const extensions::Extension* ntp_extension =
      extensions::GetExtensionOverridingNewTabPage(profile_);
  if (!ntp_extension) {
    ResolveJavascriptCallback(*callback_id, base::Value());
    return;
  }

  std::unique_ptr<base::Value> extensionInfo = GetNtpExtension();
  ResolveJavascriptCallback(*callback_id, *extensionInfo);
}

void OnStartupHandler::HandleValidateStartupPage(const base::ListValue* args) {
  CHECK_EQ(args->GetSize(), 2U);
  const base::Value* callback_id;
  CHECK(args->Get(0, &callback_id));
  std::string url_string;
  CHECK(args->GetString(1, &url_string));
  AllowJavascript();

  bool valid = settings_utils::FixUpAndValidateStartupPage(url_string, nullptr);
  ResolveJavascriptCallback(*callback_id, base::Value(valid));
}

}  // namespace settings
