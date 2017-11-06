// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/consent_auditor/consent_auditor.h"

#include <memory>

#include "base/memory/ptr_util.h"
#include "base/values.h"
#include "components/consent_auditor/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/sync/user_events/user_event_service.h"

namespace consent_auditor {

namespace {

const char kLocalConsentDescriptionKey[] = "description";
const char kLocalConsentConfirmationKey[] = "confirmation";

}  // namespace

ConsentAuditor::ConsentAuditor(PrefService* pref_service,
                               syncer::UserEventService* user_event_service)
    : pref_service_(pref_service), user_event_service_(user_event_service) {
  DCHECK(pref_service_);
  DCHECK(user_event_service_);
}

ConsentAuditor::~ConsentAuditor() {}

void ConsentAuditor::Shutdown() {}

// static
void ConsentAuditor::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(prefs::kLocalConsentsDictionary);
}

bool ConsentAuditor::RecordLocalConsent(const std::string& feature,
                                        const std::string& description_text,
                                        const std::string& confirmation_text) {
  if (!pref_service_->HasPrefPath(prefs::kLocalConsentsDictionary)) {
    pref_service_->Set(prefs::kLocalConsentsDictionary,
                       base::DictionaryValue());
  }

  {
    DictionaryPrefUpdate consents_update(pref_service_,
                                         prefs::kLocalConsentsDictionary);
    base::DictionaryValue* consents = consents_update.Get();
    DCHECK(consents);

    std::unique_ptr<base::DictionaryValue> record =
        base::MakeUnique<base::DictionaryValue>();
    record->SetString(kLocalConsentDescriptionKey, description_text);
    record->SetString(kLocalConsentConfirmationKey, confirmation_text);
    consents->SetDictionary(feature, std::move(record));
  }

  return true;
}

}  // namespace consent_auditor
