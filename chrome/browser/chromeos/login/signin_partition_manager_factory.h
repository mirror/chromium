// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SIGNIN_PARTITION_MANAGER_FACTORY_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SIGNIN_PARTITION_MANAGER_FACTORY_H_

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

namespace content {
class BrowserContext;
}

namespace chromeos {
namespace login {

class SigninPartitionManager;

class SigninPartitionManagerFactory : public BrowserContextKeyedServiceFactory {
 public:
  static SigninPartitionManager* GetForBrowserContext(
      content::BrowserContext* browser_context);

  static SigninPartitionManagerFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<SigninPartitionManagerFactory>;

  SigninPartitionManagerFactory();
  ~SigninPartitionManagerFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* context) const override;
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override;

  DISALLOW_COPY_AND_ASSIGN(SigninPartitionManagerFactory);
};

}  // namespace login
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SIGNIN_PARTITION_MANAGER_FACTORY_H_
