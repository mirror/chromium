// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/content/service_worker_payment_app_factory.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/WebKit/public/platform/modules/payments/payment_request.mojom.h"

namespace payments {

TEST(ServiceWorkerPaymentAppFactoryTest,
     RemoveAppsWithoutMatchingMethodData_NoApps) {
  std::vector<mojom::PaymentMethodDataPtr> requested_methods;
  requested_methods.emplace_back(mojom::PaymentMethodData::New());
  requested_methods.back()->supported_methods = {"method1", "method2"};
  content::PaymentAppProvider::PaymentApps no_apps;

  ServiceWorkerPaymentAppFactory::RemoveAppsWithoutMatchingMethodData(
      std::move(requested_methods), &no_apps);

  EXPECT_TRUE(no_apps.empty());
}

TEST(ServiceWorkerPaymentAppFactoryTest,
     RemoveAppsWithoutMatchingMethodData_NoMethods) {
  std::vector<mojom::PaymentMethodDataPtr> no_requested_methods;
  content::PaymentAppProvider::PaymentApps apps;
  apps[0] = std::make_unique<content::StoredPaymentApp>();
  apps[0]->enabled_methods = {"method1", "method2"};

  ServiceWorkerPaymentAppFactory::RemoveAppsWithoutMatchingMethodData(
      std::move(no_requested_methods), &apps);

  EXPECT_TRUE(apps.empty());
}

TEST(ServiceWorkerPaymentAppFactoryTest,
     RemoveAppsWithoutMatchingMethodData_IntersectionOfMethods) {
  std::vector<mojom::PaymentMethodDataPtr> requested_methods;
  requested_methods.emplace_back(mojom::PaymentMethodData::New());
  requested_methods.back()->supported_methods = {"method1"};
  requested_methods.emplace_back(mojom::PaymentMethodData::New());
  requested_methods.back()->supported_methods = {"method2"};
  requested_methods.emplace_back(mojom::PaymentMethodData::New());
  requested_methods.back()->supported_methods = {"method3"};
  content::PaymentAppProvider::PaymentApps apps;
  apps[0] = std::make_unique<content::StoredPaymentApp>();
  apps[0]->enabled_methods = {"method2"};
  apps[1] = std::make_unique<content::StoredPaymentApp>();
  apps[1]->enabled_methods = {"method3"};
  apps[2] = std::make_unique<content::StoredPaymentApp>();
  apps[2]->enabled_methods = {"method4"};

  ServiceWorkerPaymentAppFactory::RemoveAppsWithoutMatchingMethodData(
      std::move(requested_methods), &apps);

  EXPECT_EQ(2U, apps.size());
  ASSERT_NE(apps.end(), apps.find(0));
  EXPECT_EQ(std::vector<std::string>{"method2"},
            apps.find(0)->second->enabled_methods);
  ASSERT_NE(apps.end(), apps.find(1));
  EXPECT_EQ(std::vector<std::string>{"method3"},
            apps.find(1)->second->enabled_methods);
}

TEST(ServiceWorkerPaymentAppFactoryTest,
     RemoveAppsWithoutMatchingMethodData_NoNetworkCapabilities) {
  std::vector<mojom::PaymentMethodDataPtr> requested_methods;
  requested_methods.emplace_back(mojom::PaymentMethodData::New());
  requested_methods.back()->supported_methods = {"basic-card"};
  requested_methods.back()->supported_networks = {
      mojom::BasicCardNetwork::AMEX};
  content::PaymentAppProvider::PaymentApps apps;
  apps[0] = std::make_unique<content::StoredPaymentApp>();
  apps[0]->enabled_methods = {"basic-card"};

  ServiceWorkerPaymentAppFactory::RemoveAppsWithoutMatchingMethodData(
      std::move(requested_methods), &apps);

  EXPECT_TRUE(apps.empty());
}

TEST(ServiceWorkerPaymentAppFactoryTest,
     RemoveAppsWithoutMatchingMethodData_NoTypeCapabilities) {
  std::vector<mojom::PaymentMethodDataPtr> requested_methods;
  requested_methods.emplace_back(mojom::PaymentMethodData::New());
  requested_methods.back()->supported_methods = {"basic-card"};
  requested_methods.back()->supported_types = {mojom::BasicCardType::DEBIT};
  content::PaymentAppProvider::PaymentApps apps;
  apps[0] = std::make_unique<content::StoredPaymentApp>();
  apps[0]->enabled_methods = {"basic-card"};

  ServiceWorkerPaymentAppFactory::RemoveAppsWithoutMatchingMethodData(
      std::move(requested_methods), &apps);

  EXPECT_TRUE(apps.empty());
}

TEST(ServiceWorkerPaymentAppFactoryTest,
     RemoveAppsWithoutMatchingMethodData_NoMatchingNetworkCapabilities) {
  std::vector<mojom::PaymentMethodDataPtr> requested_methods;
  requested_methods.emplace_back(mojom::PaymentMethodData::New());
  requested_methods.back()->supported_methods = {"basic-card"};
  requested_methods.back()->supported_networks = {
      mojom::BasicCardNetwork::AMEX};
  content::PaymentAppProvider::PaymentApps apps;
  apps[0] = std::make_unique<content::StoredPaymentApp>();
  apps[0]->enabled_methods = {"basic-card"};
  apps[0]->capabilities.emplace_back();
  apps[0]->capabilities.back().supported_card_networks = {
      static_cast<int32_t>(mojom::BasicCardNetwork::VISA)};

  ServiceWorkerPaymentAppFactory::RemoveAppsWithoutMatchingMethodData(
      std::move(requested_methods), &apps);

  EXPECT_TRUE(apps.empty());
}

TEST(ServiceWorkerPaymentAppFactoryTest,
     RemoveAppsWithoutMatchingMethodData_NoMatchingTypeCapabilities) {
  std::vector<mojom::PaymentMethodDataPtr> requested_methods;
  requested_methods.emplace_back(mojom::PaymentMethodData::New());
  requested_methods.back()->supported_methods = {"basic-card"};
  requested_methods.back()->supported_types = {mojom::BasicCardType::DEBIT};
  content::PaymentAppProvider::PaymentApps apps;
  apps[0] = std::make_unique<content::StoredPaymentApp>();
  apps[0]->enabled_methods = {"basic-card"};
  apps[0]->capabilities.emplace_back();
  apps[0]->capabilities.back().supported_card_types = {
      static_cast<int32_t>(mojom::BasicCardType::CREDIT)};

  ServiceWorkerPaymentAppFactory::RemoveAppsWithoutMatchingMethodData(
      std::move(requested_methods), &apps);

  EXPECT_TRUE(apps.empty());
}

TEST(ServiceWorkerPaymentAppFactoryTest,
     RemoveAppsWithoutMatchingMethodData_NoRequestedNetworkOrType) {
  std::vector<mojom::PaymentMethodDataPtr> requested_methods;
  requested_methods.emplace_back(mojom::PaymentMethodData::New());
  requested_methods.back()->supported_methods = {"basic-card"};
  content::PaymentAppProvider::PaymentApps apps;
  apps[0] = std::make_unique<content::StoredPaymentApp>();
  apps[0]->enabled_methods = {"basic-card"};
  apps[0]->capabilities.emplace_back();
  apps[0]->capabilities.back().supported_card_networks = {
      static_cast<int32_t>(mojom::BasicCardNetwork::VISA)};
  apps[0]->capabilities.back().supported_card_types = {
      static_cast<int32_t>(mojom::BasicCardType::CREDIT)};

  ServiceWorkerPaymentAppFactory::RemoveAppsWithoutMatchingMethodData(
      std::move(requested_methods), &apps);

  EXPECT_EQ(1U, apps.size());
  ASSERT_NE(apps.end(), apps.find(0));
  const auto& actual = apps.find(0)->second;
  EXPECT_EQ(std::vector<std::string>{"basic-card"}, actual->enabled_methods);
  ASSERT_EQ(1U, actual->capabilities.size());
  const auto& capability = actual->capabilities.back();
  ASSERT_EQ(1U, capability.supported_card_types.size());
  EXPECT_EQ(static_cast<int32_t>(mojom::BasicCardType::CREDIT),
            capability.supported_card_types[0]);
  ASSERT_EQ(1U, actual->capabilities.back().supported_card_networks.size());
  EXPECT_EQ(static_cast<int32_t>(mojom::BasicCardNetwork::VISA),
            capability.supported_card_networks[0]);
}

TEST(ServiceWorkerPaymentAppFactoryTest,
     RemoveAppsWithoutMatchingMethodData_IntersectionOfNetworksAndTypes) {
  std::vector<mojom::PaymentMethodDataPtr> requested_methods;
  requested_methods.emplace_back(mojom::PaymentMethodData::New());
  requested_methods.back()->supported_methods = {"basic-card"};
  requested_methods.back()->supported_types = {mojom::BasicCardType::DEBIT,
                                               mojom::BasicCardType::CREDIT};
  requested_methods.back()->supported_networks = {
      mojom::BasicCardNetwork::AMEX, mojom::BasicCardNetwork::DINERS};
  content::PaymentAppProvider::PaymentApps apps;
  apps[0] = std::make_unique<content::StoredPaymentApp>();
  apps[0]->enabled_methods = {"basic-card"};
  apps[0]->capabilities.emplace_back();
  apps[0]->capabilities.back().supported_card_networks = {
      static_cast<int32_t>(mojom::BasicCardNetwork::DINERS),
      static_cast<int32_t>(mojom::BasicCardNetwork::VISA)};
  apps[0]->capabilities.back().supported_card_types = {
      static_cast<int32_t>(mojom::BasicCardType::PREPAID),
      static_cast<int32_t>(mojom::BasicCardType::DEBIT)};

  ServiceWorkerPaymentAppFactory::RemoveAppsWithoutMatchingMethodData(
      std::move(requested_methods), &apps);

  EXPECT_EQ(1U, apps.size());
  ASSERT_NE(apps.end(), apps.find(0));
  const auto& actual = apps.find(0)->second;
  EXPECT_EQ(std::vector<std::string>{"basic-card"}, actual->enabled_methods);
  ASSERT_EQ(1U, actual->capabilities.size());
  const auto& capability = actual->capabilities.back();
  EXPECT_EQ(
      (std::vector<int32_t>{static_cast<int32_t>(mojom::BasicCardType::PREPAID),
                            static_cast<int32_t>(mojom::BasicCardType::DEBIT)}),
      capability.supported_card_types);
  EXPECT_EQ((std::vector<int32_t>{
                static_cast<int32_t>(mojom::BasicCardNetwork::DINERS),
                static_cast<int32_t>(mojom::BasicCardNetwork::VISA)}),
            capability.supported_card_networks);
}

TEST(ServiceWorkerPaymentAppFactoryTest,
     RemoveAppsWithoutMatchingMethodData_NonBasicCardIgnoresCapabilities) {
  std::vector<mojom::PaymentMethodDataPtr> requested_methods;
  requested_methods.emplace_back(mojom::PaymentMethodData::New());
  requested_methods.back()->supported_methods = {"unknown-method"};
  requested_methods.back()->supported_types = {mojom::BasicCardType::DEBIT};
  content::PaymentAppProvider::PaymentApps apps;
  apps[0] = std::make_unique<content::StoredPaymentApp>();
  apps[0]->enabled_methods = {"unknown-method"};
  apps[0]->capabilities.emplace_back();
  apps[0]->capabilities.back().supported_card_types = {
      static_cast<int32_t>(mojom::BasicCardType::PREPAID)};

  ServiceWorkerPaymentAppFactory::RemoveAppsWithoutMatchingMethodData(
      std::move(requested_methods), &apps);

  EXPECT_EQ(1U, apps.size());
  ASSERT_NE(apps.end(), apps.find(0));
  EXPECT_EQ(std::vector<std::string>{"unknown-method"},
            apps.find(0)->second->enabled_methods);
}

}  // namespace payments
