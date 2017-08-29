// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/subresource_filter/subresource_filter_failsafe.h"

#include <vector>

#include "base/macros.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/base/testing_profile.h"
#include "components/subresource_filter/core/browser/subresource_filter_features.h"
#include "components/subresource_filter/core/browser/subresource_filter_features_test_support.h"
#include "components/subresource_filter/core/common/activation_level.h"
#include "components/subresource_filter/core/common/activation_scope.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using subresource_filter::testing::ScopedSubresourceFilterFeatureToggle;
using subresource_filter::testing::ScopedSubresourceFilterConfigurator;

class SubresourceFilterFailsafeTest : public testing::Test {
 public:
  SubresourceFilterFailsafeTest() {}
  ~SubresourceFilterFailsafeTest() override {}

  TestingProfile* profile() { return &profile_; }

 private:
  content::TestBrowserThreadBundle thread_bundle_;
  TestingProfile profile_;
  DISALLOW_COPY_AND_ASSIGN(SubresourceFilterFailsafeTest);
};

TEST_F(SubresourceFilterFailsafeTest, NoFeature_NoFailsafe) {
  ScopedSubresourceFilterFeatureToggle feature_toggle(
      base::FeatureList::OVERRIDE_ENABLE_FEATURE, "",
      "SubresourceFilterFailsafe");

  subresource_filter::Configuration config(
      subresource_filter::ActivationLevel::ENABLED,
      subresource_filter::ActivationScope::ALL_SITES);
  ScopedSubresourceFilterConfigurator configurator(config);

  SubresourceFilterFailsafe failsafe(profile());
  const GURL new_tab_url(chrome::kChromeSearchLocalNtpUrl);
  failsafe.OnDidShowUI(new_tab_url, config);

  EXPECT_TRUE(subresource_filter::HasEnabledConfiguration(config));
}

TEST_F(SubresourceFilterFailsafeTest, ActivateNewTab_TriggersFailsafe) {
  ScopedSubresourceFilterFeatureToggle feature_toggle(
      base::FeatureList::OVERRIDE_ENABLE_FEATURE, "SubresourceFilterFailsafe",
      "");
  subresource_filter::Configuration config(
      subresource_filter::ActivationLevel::ENABLED,
      subresource_filter::ActivationScope::ALL_SITES);
  ScopedSubresourceFilterConfigurator configurator(config);

  SubresourceFilterFailsafe failsafe(profile());
  const GURL new_tab_url(chrome::kChromeSearchLocalNtpUrl);
  failsafe.OnDidShowUI(new_tab_url, config);

  EXPECT_FALSE(subresource_filter::HasEnabledConfiguration(config));
}

TEST_F(SubresourceFilterFailsafeTest, FailsafeWithMultipleConfigurations) {
  ScopedSubresourceFilterFeatureToggle feature_toggle(
      base::FeatureList::OVERRIDE_ENABLE_FEATURE, "SubresourceFilterFailsafe",
      "");
  std::vector<subresource_filter::Configuration> configs = {
      subresource_filter::Configuration::
          MakePresetForPerformanceTestingDryRunOnAllSites(),
      subresource_filter::Configuration::MakePresetForLiveRunOnPhishingSites(),
      subresource_filter::Configuration(
          subresource_filter::ActivationLevel::ENABLED,
          subresource_filter::ActivationScope::ALL_SITES)};
  ScopedSubresourceFilterConfigurator configurator(configs);

  const subresource_filter::Configuration matching_config = configs.back();
  SubresourceFilterFailsafe failsafe(profile());
  const GURL new_tab_url(chrome::kChromeSearchLocalNtpUrl);
  failsafe.OnDidShowUI(new_tab_url, matching_config);

  EXPECT_FALSE(subresource_filter::HasEnabledConfiguration(matching_config));
  EXPECT_EQ(2u, subresource_filter::GetEnabledConfigurations()
                    ->configs_by_decreasing_priority()
                    .size());
}
