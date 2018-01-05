// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/settings_private/generated_prefs_factory.h"

#include "chrome/browser/extensions/api/settings_private/generated_prefs.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace extensions {
namespace settings_private {

using content::BrowserContext;

// static
GeneratedPrefs* GeneratedPrefsFactory::GetForBrowserContext(
    BrowserContext* browser_context) {
  return static_cast<GeneratedPrefs*>(
      GetInstance()->GetServiceForBrowserContext(browser_context, true));
}

// static
GeneratedPrefsFactory* GeneratedPrefsFactory::GetInstance() {
  return base::Singleton<GeneratedPrefsFactory>::get();
}

GeneratedPrefsFactory::GeneratedPrefsFactory()
    : BrowserContextKeyedServiceFactory(
          "GeneratedPrefs",
          BrowserContextDependencyManager::GetInstance()) {}

GeneratedPrefsFactory::~GeneratedPrefsFactory() {}

content::BrowserContext* GeneratedPrefsFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  // Use the incognito profile when in Guest mode.
  if (context->IsOffTheRecord())
    return chrome::GetBrowserContextRedirectedInIncognito(context);
  return context;
}

KeyedService* GeneratedPrefsFactory::BuildServiceInstanceFor(
    content::BrowserContext* profile) const {
  return new GeneratedPrefs(static_cast<Profile*>(profile));
}

}  // namespace settings_private
}  // namespace extensions
