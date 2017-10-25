// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/content/service_worker_payment_app_factory.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace payments {

class ServiceWorkerPaymentAppFactoryTest : public testing::Test {
 public:
  ServiceWorkerPaymentAppFactoryTest() {}
  ~ServiceWorkerPaymentAppFactoryTest() override {}

  void RemoveAppsWithoutMatchingMethods(
      const std::set<std::string>& requested_methods,
      content::PaymentAppProvider::PaymentApps* apps) {
    ServiceWorkerPaymentAppFactory::RemoveAppsWithoutMatchingMethods(
        requested_methods, apps);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerPaymentAppFactoryTest);
};

TEST_F(ServiceWorkerPaymentAppFactoryTest,
       RemoveAppsWithoutMatchingMethods_NoApps) {
  std::set<std::string> requested_methods = {"method1", "method2"};
  content::PaymentAppProvider::PaymentApps no_apps;

  RemoveAppsWithoutMatchingMethods(requested_methods, &no_apps);

  EXPECT_TRUE(no_apps.empty());
}

TEST_F(ServiceWorkerPaymentAppFactoryTest,
       RemoveAppsWithoutMatchingMethods_NoMethods) {
  std::set<std::string> no_requested_methods;
  content::PaymentAppProvider::PaymentApps apps;
  apps[0] = std::make_unique<content::StoredPaymentApp>();
  apps[0]->enabled_methods = {"method1", "method2"};

  RemoveAppsWithoutMatchingMethods(no_requested_methods, &apps);

  EXPECT_TRUE(apps.empty());
}

TEST_F(ServiceWorkerPaymentAppFactoryTest,
       RemoveAppsWithoutMatchingMethods_IntersectionOfMethods) {
  std::set<std::string> requested_methods = {"method1", "method2", "method3"};
  content::PaymentAppProvider::PaymentApps apps;
  apps[0] = std::make_unique<content::StoredPaymentApp>();
  apps[0]->enabled_methods = {"method2"};
  apps[1] = std::make_unique<content::StoredPaymentApp>();
  apps[1]->enabled_methods = {"method3"};
  apps[2] = std::make_unique<content::StoredPaymentApp>();
  apps[2]->enabled_methods = {"method4"};

  RemoveAppsWithoutMatchingMethods(requested_methods, &apps);

  EXPECT_EQ(2U, apps.size());
  ASSERT_NE(apps.end(), apps.find(0));
  EXPECT_EQ(std::vector<std::string>{"method2"},
            apps.find(0)->second->enabled_methods);
  ASSERT_NE(apps.end(), apps.find(1));
  EXPECT_EQ(std::vector<std::string>{"method3"},
            apps.find(1)->second->enabled_methods);
}

}  // namespace payments
