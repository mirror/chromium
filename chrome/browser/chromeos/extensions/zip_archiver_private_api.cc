// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/extensions/zip_archiver_private_api.h"

#include <memory>
#include <utility>

#include "chrome/browser/browser_process.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/webui/web_ui_util.h"

namespace {

#define SET_STRING(id, idr) dict->SetString(id, l10n_util::GetStringUTF16(idr))

}  // namespace

ExtensionFunction::ResponseAction
ZipArchiverPrivateGetLocalizedStringsFunction::Run() {
  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue());

  SET_STRING("passphraseTitle", IDS_ZIPARCHIVER_PASSPHRASE_TITLE);
  SET_STRING("passphraseInputLabel", IDS_ZIPARCHIVER_PASSPHRASE_INPUT_LABEL);
  SET_STRING("passphraseAccept", IDS_ZIPARCHIVER_PASSPHRASE_ACCEPT);
  SET_STRING("passphraseCancel", IDS_ZIPARCHIVER_PASSPHRASE_CANCEL);
  SET_STRING("passphraseRemember", IDS_ZIPARCHIVER_PASSPHRASE_REMEMBER);

  const std::string& app_locale = g_browser_process->GetApplicationLocale();
  webui::SetLoadTimeDataDefaults(app_locale, dict.get());

  return RespondNow(OneArgument(std::move(dict)));
}
