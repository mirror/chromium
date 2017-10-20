// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/consent_auditor/consent_auditor.h"

#include "base/memory.h"
#include "base/values.h"
#include "components/prefs/pref_service.h"

namespace consent_auditor {

namespace {

const char kLocalConsentDescriptionKey[] = "description";
const char kLocalConsentConfirmationKey[] = "confirmation";

}  // namespace

bool RecordLocalConsent(Profile* profile,
                        const std::string& feature,
                        const std::string& description_text,
                        const std::string& confirmation_text) {
  PrefService* prefs = profile.GetPrefs();

  if (!prefs->HasPrefPath(prefs::kLocalConsents)) {
    prefs->SetDictionary(prefs::kLocalConsents,
                         base::MakeUnique<base::DictionaryValue>());
  }

  {
    DictionaryPrefUpdate consents_update(prefs, prefs::kLocalConsents);
    base::DictionaryValue* consents = consents_update.Get();
    DCHECK(consents);

    base::DictionaryValue* record = base::MakeUnique<base::DictionaryValue>();
    record->SetString(kLocalConsentDescriptionKey, description_text);
    record->SetString(kLocalConsentConfirmationKey, confirmation_text);
    consents->SetDictionary(record);
  }

  return true;
}

}  // namespace consent_auditor
