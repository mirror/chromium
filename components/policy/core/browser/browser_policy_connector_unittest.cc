// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/policy/core/browser/browser_policy_connector.h"

#include <memory>

#include "base/bind.h"
#include "base/macros.h"
#include "components/policy/core/common/mock_configuration_policy_provider.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace policy {

TEST(BrowserPolicyConnectorTest, IsNonEnterpriseUser) {
  // List of example emails that are not enterprise users.
  static const char* kNonEnterpriseUsers[] = {
    "fizz@aol.com",
    "foo@gmail.com",
    "bar@googlemail.com",
    "baz@hotmail.it",
    "baz@hotmail.co.uk",
    "baz@hotmail.com.tw",
    "user@msn.com",
    "another_user@live.com",
    "foo@qq.com",
    "i_love@yahoo.com",
    "i_love@yahoo.com.tw",
    "i_love@yahoo.jp",
    "i_love@yahoo.co.uk",
    "user@yandex.ru"
  };

  // List of example emails that are potential enterprise users.
  static const char* kEnterpriseUsers[] = {
    "foo@google.com",
    "chrome_rules@chromium.org",
    "user@hotmail.enterprise.com",
  };

  for (unsigned int i = 0; i < arraysize(kNonEnterpriseUsers); ++i) {
    std::string username(kNonEnterpriseUsers[i]);
    EXPECT_TRUE(BrowserPolicyConnector::IsNonEnterpriseUser(username)) <<
        "IsNonEnterpriseUser returned false for " << username;
  }
  for (unsigned int i = 0; i < arraysize(kEnterpriseUsers); ++i) {
    std::string username(kEnterpriseUsers[i]);
    EXPECT_FALSE(BrowserPolicyConnector::IsNonEnterpriseUser(username)) <<
        "IsNonEnterpriseUser returned true for " << username;
  }
}

void PopulatePolicyHandlerParameters(PolicyHandlerParameters* params) {}

std::unique_ptr<ConfigurationPolicyHandlerList>
CreateConfigurationPolicyHandlerList(const Schema& schema) {
  return std::make_unique<ConfigurationPolicyHandlerList>(
      base::Bind(&PopulatePolicyHandlerParameters),
      GetChromePolicyDetailsCallback());
}

class TestBrowserPolicyConnector : public BrowserPolicyConnector {
 public:
  TestBrowserPolicyConnector()
      : BrowserPolicyConnector(
            base::Bind(&CreateConfigurationPolicyHandlerList)) {}
  ~TestBrowserPolicyConnector() override = default;

  // Exposed so tests can call these functions.
  void InitPolicyProviders() { BrowserPolicyConnector::InitPolicyProviders(); }
  void AddPolicyProvider(
      std::unique_ptr<ConfigurationPolicyProvider> provider) {
    BrowserPolicyConnector::AddPolicyProvider(std::move(provider));
  }

  // BrowserPolicyConnector:
  void Init(
      PrefService* local_state,
      scoped_refptr<net::URLRequestContextGetter> request_context) override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(TestBrowserPolicyConnector);
};

TEST(BrowserPolicyConnector, AddPolicyProviderAfterServiceCreated) {
  TestBrowserPolicyConnector connector;
  connector.InitPolicyProviders();
  std::unique_ptr<MockConfigurationPolicyProvider> mock_provider =
      std::make_unique<MockConfigurationPolicyProvider>();
  EXPECT_CALL(*mock_provider, IsInitializationComplete(testing::_))
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::Return(true));
  connector.GetPolicyService();
  connector.AddPolicyProvider(std::move(mock_provider));
}

}  // namespace policy
