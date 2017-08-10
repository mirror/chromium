// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/fileapi/arc_documents_provider_root_map_factory.h"

#include <algorithm>
#include <iterator>
#include <string>
#include <utility>

#include "chrome/browser/chromeos/arc/arc_util.h"
#include "chrome/browser/chromeos/arc/fileapi/arc_documents_provider_root_map.h"
#include "chrome/browser/chromeos/arc/fileapi/arc_file_system_operation_runner.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace arc {

// static
ArcDocumentsProviderRootMap* ArcDocumentsProviderRootMapFactory::GetForProfile(
    Profile* profile) {
  return static_cast<ArcDocumentsProviderRootMap*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

ArcDocumentsProviderRootMapFactory::ArcDocumentsProviderRootMapFactory()
    : BrowserContextKeyedServiceFactory(
          "ArcDocumentsProviderRootMap",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(ArcFileSystemOperationRunner::GetFactory());
}

ArcDocumentsProviderRootMapFactory::~ArcDocumentsProviderRootMapFactory() =
    default;

// static
ArcDocumentsProviderRootMapFactory*
ArcDocumentsProviderRootMapFactory::GetInstance() {
  return base::Singleton<ArcDocumentsProviderRootMapFactory>::get();
}

content::BrowserContext*
ArcDocumentsProviderRootMapFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

KeyedService* ArcDocumentsProviderRootMapFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);

  // Check if ARC++ is allowed for this profile.
  if (!arc::IsArcAllowedForProfile(profile))
    return nullptr;

  return new ArcDocumentsProviderRootMap(profile);
}

}  // namespace arc
