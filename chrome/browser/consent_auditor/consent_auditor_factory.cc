// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/consent_auditor/consent_auditor_factory.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/user_event_service_factory.h"
#include "components/consent_auditor/consent_auditor.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/prefs/pref_service.h"

// static
ConsentAuditorFactory* ConsentAuditorFactory::GetInstance() {
  return base::Singleton<ConsentAuditorFactory>::get();
}

// static
consent_auditor::ConsentAuditor* ConsentAuditorFactory::GetForProfile(
    Profile* profile) {
  return static_cast<consent_auditor::ConsentAuditor*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

ConsentAuditorFactory::ConsentAuditorFactory()
    : BrowserContextKeyedServiceFactory(
          "ConsentAuditor",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(browser_sync::UserEventServiceFactory::GetInstance());
}

ConsentAuditorFactory::~ConsentAuditorFactory() {}

content::BrowserContext* ConsentAuditorFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  // Recording local consents in Incognito is not useful, as the record would
  // soon disappear. Consents tied to the user's Google account should retrieve
  // account information from the original profile. In both cases, there is no
  // reason to support Incognito.
  Profile* profile = static_cast<Profile*>(context);
  DCHECK(!profile->IsOffTheRecord());

  return profile;
}

KeyedService* ConsentAuditorFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  Profile* profile = static_cast<Profile*>(context);
  return new consent_auditor::ConsentAuditor(
      profile->GetPrefs(),
      browser_sync::UserEventServiceFactory::GetForProfile(profile));
}
