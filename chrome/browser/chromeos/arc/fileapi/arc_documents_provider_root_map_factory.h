// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_ARC_FILEAPI_ARC_DOCUMENTS_PROVIDER_ROOT_MAP_FACTORY_H_
#define CHROME_BROWSER_CHROMEOS_ARC_FILEAPI_ARC_DOCUMENTS_PROVIDER_ROOT_MAP_FACTORY_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class Profile;

namespace arc {

class ArcDocumentsProviderRootMap;

class ArcDocumentsProviderRootMapFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  // Returns the ArcDocumentsProviderRootMap for |profile|, creating it if not
  // created yet.
  static ArcDocumentsProviderRootMap* GetForProfile(Profile* profile);

  // Returns the singleton ArcDocumentsProviderRootMapFactory instance.
  static ArcDocumentsProviderRootMapFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<
      ArcDocumentsProviderRootMapFactory>;

  ArcDocumentsProviderRootMapFactory();
  ~ArcDocumentsProviderRootMapFactory() override;

  // BrowserContextKeyedServiceFactory overrides.
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(ArcDocumentsProviderRootMapFactory);
};

}  // namespace arc

#endif  // CHROME_BROWSER_CHROMEOS_ARC_FILEAPI_ARC_DOCUMENTS_PROVIDER_ROOT_MAP_FACTORY_H_
