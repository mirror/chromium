// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_DELEGATE_FACTORY_H_
#define CHROME_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_DELEGATE_FACTORY_H_

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class Profile;

namespace background_fetch {
class BackgroundFetchDelegateImpl;
}  // namespace background_fetch

class BackgroundFetchDelegateFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static background_fetch::BackgroundFetchDelegateImpl* GetForProfile(
      Profile* profile);
  static BackgroundFetchDelegateFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<BackgroundFetchDelegateFactory>;

  BackgroundFetchDelegateFactory();
  ~BackgroundFetchDelegateFactory() override;

  // BrowserContextKeyedBaseFactory methods:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  // content::BrowserContext* GetBrowserContextToUse(
  //     content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(BackgroundFetchDelegateFactory);
};

#endif  // CHROME_BROWSER_BACKGROUND_FETCH_BACKGROUND_FETCH_DELEGATE_FACTORY_H_
