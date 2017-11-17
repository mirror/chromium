// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/signin_partition_manager_factory.h"

#include "chrome/browser/chromeos/login/signin_partition_manager.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "content/public/browser/browser_context.h"

namespace chromeos {
namespace login {

SigninPartitionManagerFactory::SigninPartitionManagerFactory()
    : BrowserContextKeyedServiceFactory(
          "SigninPartitionManager",
          BrowserContextDependencyManager::GetInstance()) {}

SigninPartitionManagerFactory::~SigninPartitionManagerFactory() {}

// static
SigninPartitionManager* SigninPartitionManagerFactory::GetForBrowserContext(
    content::BrowserContext* browser_context) {
  return static_cast<SigninPartitionManager*>(
      GetInstance()->GetServiceForBrowserContext(browser_context,
                                                 true /* create */));
}

// static
SigninPartitionManagerFactory* SigninPartitionManagerFactory::GetInstance() {
  return base::Singleton<SigninPartitionManagerFactory>::get();
}

KeyedService* SigninPartitionManagerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new SigninPartitionManager(context);
}

content::BrowserContext* SigninPartitionManagerFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  // Create a separate instance of the service for the Incognito context.
  return context;
}

}  // namespace login
}  // namespace chromeos
