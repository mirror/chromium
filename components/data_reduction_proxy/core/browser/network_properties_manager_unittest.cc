// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_reduction_proxy/core/browser/network_properties_manager.h"

#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/histogram_tester.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace data_reduction_proxy {

namespace {

class TestNetworkPropertiesManager : public NetworkPropertiesManager {
 public:
  TestNetworkPropertiesManager(
      PrefService* pref_service,
      scoped_refptr<base::SequencedTaskRunner> ui_task_runner,
      scoped_refptr<base::SingleThreadTaskRunner> io_task_runner)
      : NetworkPropertiesManager(pref_service, ui_task_runner, io_task_runner) {

  }
  ~TestNetworkPropertiesManager() override {}

  using NetworkPropertiesManager::ParsedPrefs;
  using NetworkPropertiesManager::ForceReadPrefsForTesting;
};

TEST(NetworkPropertyTest, TestSetterGetterCaptivePortal) {
  base::HistogramTester histogram_tester;
  TestingPrefServiceSimple test_prefs;
  test_prefs.registry()->RegisterDictionaryPref(prefs::kNetworkProperties);
  base::MessageLoopForIO loop;
  TestNetworkPropertiesManager network_properties_manager(
      &test_prefs, base::ThreadTaskRunnerHandle::Get(),
      base::ThreadTaskRunnerHandle::Get());

  histogram_tester.ExpectUniqueSample(
      "DataReductionProxy.NetworkProperties.CountAtStartup", 0, 1);

  std::string network_id("test");
  network_properties_manager.OnChangeInNetworkID(network_id);

  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed());
  EXPECT_TRUE(network_properties_manager.IsSecureProxyAllowed());

  network_properties_manager.SetIsCaptivePortal(true);
  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed());
  EXPECT_FALSE(network_properties_manager.IsSecureProxyAllowed());
  EXPECT_FALSE(network_properties_manager.IsSecureProxyDisallowedByCarrier());
  EXPECT_TRUE(network_properties_manager.IsCaptivePortal());
  EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(false));
  EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(true));
  base::RunLoop().RunUntilIdle();

  // Verify the prefs.
  TestNetworkPropertiesManager::ParsedPrefs parsed_prefs =
      network_properties_manager.ForceReadPrefsForTesting();
  ASSERT_EQ(1u, parsed_prefs.size());
  EXPECT_EQ(parsed_prefs.end(), parsed_prefs.find("mismatched_network_id"));
  EXPECT_NE(parsed_prefs.end(), parsed_prefs.find(network_id));
  EXPECT_TRUE(parsed_prefs.find(network_id)->second.has_captive_portal());

  network_properties_manager.SetIsCaptivePortal(false);
  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed());
  EXPECT_TRUE(network_properties_manager.IsSecureProxyAllowed());
  EXPECT_FALSE(network_properties_manager.IsSecureProxyDisallowedByCarrier());
  EXPECT_FALSE(network_properties_manager.IsCaptivePortal());
  EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(false));
  EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(true));
  base::RunLoop().RunUntilIdle();

  // Verify the prefs.
  parsed_prefs = network_properties_manager.ForceReadPrefsForTesting();
  ASSERT_EQ(1u, parsed_prefs.size());
  EXPECT_EQ(parsed_prefs.end(), parsed_prefs.find("mismatched_network_id"));
  EXPECT_NE(parsed_prefs.end(), parsed_prefs.find(network_id));
  EXPECT_FALSE(parsed_prefs.find(network_id)->second.has_captive_portal());
}

TEST(NetworkPropertyTest, TestSetterGetterDisallowedByCarrier) {
  TestingPrefServiceSimple test_prefs;
  test_prefs.registry()->RegisterDictionaryPref(prefs::kNetworkProperties);
  base::MessageLoopForIO loop;
  TestNetworkPropertiesManager network_properties_manager(
      &test_prefs, base::ThreadTaskRunnerHandle::Get(),
      base::ThreadTaskRunnerHandle::Get());

  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed());
  EXPECT_TRUE(network_properties_manager.IsSecureProxyAllowed());

  network_properties_manager.SetIsSecureProxyDisallowedByCarrier(true);
  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed());
  EXPECT_FALSE(network_properties_manager.IsSecureProxyAllowed());
  EXPECT_TRUE(network_properties_manager.IsSecureProxyDisallowedByCarrier());
  EXPECT_FALSE(network_properties_manager.IsCaptivePortal());
  EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(false));
  EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(true));

  network_properties_manager.SetIsSecureProxyDisallowedByCarrier(false);
  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed());
  EXPECT_TRUE(network_properties_manager.IsSecureProxyAllowed());
  EXPECT_FALSE(network_properties_manager.IsSecureProxyDisallowedByCarrier());
  EXPECT_FALSE(network_properties_manager.IsCaptivePortal());
  EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(false));
  EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(true));
}

TEST(NetworkPropertyTest, TestWarmupURLFailedOnSecureProxy) {
  TestingPrefServiceSimple test_prefs;
  test_prefs.registry()->RegisterDictionaryPref(prefs::kNetworkProperties);
  base::MessageLoopForIO loop;
  TestNetworkPropertiesManager network_properties_manager(
      &test_prefs, base::ThreadTaskRunnerHandle::Get(),
      base::ThreadTaskRunnerHandle::Get());

  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed());
  EXPECT_TRUE(network_properties_manager.IsSecureProxyAllowed());

  network_properties_manager.SetHasWarmupURLProbeFailed(true, true);
  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed());
  EXPECT_FALSE(network_properties_manager.IsSecureProxyAllowed());
  EXPECT_FALSE(network_properties_manager.IsSecureProxyDisallowedByCarrier());
  EXPECT_FALSE(network_properties_manager.IsCaptivePortal());
  EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(false));
  EXPECT_TRUE(network_properties_manager.HasWarmupURLProbeFailed(true));

  network_properties_manager.SetHasWarmupURLProbeFailed(true, false);
  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed());
  EXPECT_TRUE(network_properties_manager.IsSecureProxyAllowed());
  EXPECT_FALSE(network_properties_manager.IsSecureProxyDisallowedByCarrier());
  EXPECT_FALSE(network_properties_manager.IsCaptivePortal());
  EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(false));
  EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(true));
}

TEST(NetworkPropertyTest, TestWarmupURLFailedOnInSecureProxy) {
  TestingPrefServiceSimple test_prefs;
  test_prefs.registry()->RegisterDictionaryPref(prefs::kNetworkProperties);
  base::MessageLoopForIO loop;
  TestNetworkPropertiesManager network_properties_manager(
      &test_prefs, base::ThreadTaskRunnerHandle::Get(),
      base::ThreadTaskRunnerHandle::Get());

  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed());
  EXPECT_TRUE(network_properties_manager.IsSecureProxyAllowed());

  network_properties_manager.SetHasWarmupURLProbeFailed(false, true);
  EXPECT_FALSE(network_properties_manager.IsInsecureProxyAllowed());
  EXPECT_TRUE(network_properties_manager.IsSecureProxyAllowed());
  EXPECT_FALSE(network_properties_manager.IsSecureProxyDisallowedByCarrier());
  EXPECT_FALSE(network_properties_manager.IsCaptivePortal());
  EXPECT_TRUE(network_properties_manager.HasWarmupURLProbeFailed(false));
  EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(true));

  network_properties_manager.SetHasWarmupURLProbeFailed(false, false);
  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed());
  EXPECT_TRUE(network_properties_manager.IsSecureProxyAllowed());
  EXPECT_FALSE(network_properties_manager.IsSecureProxyDisallowedByCarrier());
  EXPECT_FALSE(network_properties_manager.IsCaptivePortal());
  EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(false));
  EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(true));
}

TEST(NetworkPropertyTest, TestLimitPrefSize) {
  TestingPrefServiceSimple test_prefs;
  test_prefs.registry()->RegisterDictionaryPref(prefs::kNetworkProperties);
  base::MessageLoopForIO loop;
  TestNetworkPropertiesManager network_properties_manager(
      &test_prefs, base::ThreadTaskRunnerHandle::Get(),
      base::ThreadTaskRunnerHandle::Get());

  size_t num_network_ids = 100;

  for (size_t i = 0; i < num_network_ids; ++i) {
    std::string network_id("test" + base::IntToString(i));
    network_properties_manager.OnChangeInNetworkID(network_id);

    // State should be reset when there is a change in the network ID.
    EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed());
    EXPECT_TRUE(network_properties_manager.IsSecureProxyAllowed());

    network_properties_manager.SetIsCaptivePortal(true);
    EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed());
    EXPECT_FALSE(network_properties_manager.IsSecureProxyAllowed());
    EXPECT_FALSE(network_properties_manager.IsSecureProxyDisallowedByCarrier());
    EXPECT_TRUE(network_properties_manager.IsCaptivePortal());
    EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(false));
    EXPECT_FALSE(network_properties_manager.HasWarmupURLProbeFailed(true));

    base::RunLoop().RunUntilIdle();
  }

  TestNetworkPropertiesManager::ParsedPrefs parsed_prefs =
      network_properties_manager.ForceReadPrefsForTesting();
  // Pref size should be bounded to 10.
  ASSERT_EQ(10u, parsed_prefs.size());
  EXPECT_EQ(parsed_prefs.end(), parsed_prefs.find("mismatched_network_id"));

  // The last network ID is guaranteed to be present in the prefs.
  EXPECT_NE(parsed_prefs.end(),
            parsed_prefs.find("test" + base::IntToString(num_network_ids - 1)));

  for (const auto& network_property : parsed_prefs) {
    EXPECT_TRUE(network_property.second.has_captive_portal());
    EXPECT_FALSE(network_property.second.secure_proxy_disallowed_by_carrier());
    EXPECT_FALSE(network_property.second.secure_proxy_flags()
                     .disallowed_due_to_warmup_probe_failure());
    EXPECT_FALSE(network_property.second.insecure_proxy_flags()
                     .disallowed_due_to_warmup_probe_failure());
  }
}

TEST(NetworkPropertyTest, TestChangeNetworkIDBackAndForth) {
  TestingPrefServiceSimple test_prefs;
  test_prefs.registry()->RegisterDictionaryPref(prefs::kNetworkProperties);
  base::MessageLoopForIO loop;
  TestNetworkPropertiesManager network_properties_manager(
      &test_prefs, base::ThreadTaskRunnerHandle::Get(),
      base::ThreadTaskRunnerHandle::Get());

  // First network ID has a captive portal.
  std::string first_network_id("test1");
  network_properties_manager.OnChangeInNetworkID(first_network_id);
  // State should be reset when there is a change in the network ID.
  EXPECT_FALSE(network_properties_manager.IsCaptivePortal());
  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed());
  network_properties_manager.SetIsCaptivePortal(true);
  EXPECT_TRUE(network_properties_manager.IsCaptivePortal());
  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed());
  base::RunLoop().RunUntilIdle();

  // Warmup probe failed on the insecure proxy for the second network ID.
  std::string second_network_id("test2");
  network_properties_manager.OnChangeInNetworkID(second_network_id);
  // State should be reset when there is a change in the network ID.
  EXPECT_FALSE(network_properties_manager.IsCaptivePortal());
  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed());
  network_properties_manager.SetHasWarmupURLProbeFailed(false, true);
  EXPECT_FALSE(network_properties_manager.IsCaptivePortal());
  EXPECT_FALSE(network_properties_manager.IsInsecureProxyAllowed());
  base::RunLoop().RunUntilIdle();

  // Change back to |first_network_id|.
  network_properties_manager.OnChangeInNetworkID(first_network_id);
  EXPECT_TRUE(network_properties_manager.IsCaptivePortal());
  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed());

  // Change back to |second_network_id|.
  network_properties_manager.OnChangeInNetworkID(second_network_id);
  EXPECT_FALSE(network_properties_manager.IsCaptivePortal());
  EXPECT_FALSE(network_properties_manager.IsInsecureProxyAllowed());

  // Verify the prefs.
  TestNetworkPropertiesManager::ParsedPrefs parsed_prefs =
      network_properties_manager.ForceReadPrefsForTesting();
  ASSERT_EQ(2u, parsed_prefs.size());
  EXPECT_EQ(parsed_prefs.end(), parsed_prefs.find("mismatched_network_id"));
  EXPECT_NE(parsed_prefs.end(), parsed_prefs.find(first_network_id));
  EXPECT_NE(parsed_prefs.end(), parsed_prefs.find(second_network_id));
}

TEST(NetworkPropertyTest, TestNetworkQualitiesOverwrite) {
  TestingPrefServiceSimple test_prefs;
  test_prefs.registry()->RegisterDictionaryPref(prefs::kNetworkProperties);
  base::MessageLoopForIO loop;
  TestNetworkPropertiesManager network_properties_manager(
      &test_prefs, base::ThreadTaskRunnerHandle::Get(),
      base::ThreadTaskRunnerHandle::Get());

  // First network ID has a captive portal.
  std::string first_network_id("test1");
  network_properties_manager.OnChangeInNetworkID(first_network_id);
  // State should be reset when there is a change in the network ID.
  EXPECT_FALSE(network_properties_manager.IsCaptivePortal());
  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed());
  network_properties_manager.SetIsCaptivePortal(true);
  EXPECT_TRUE(network_properties_manager.IsCaptivePortal());
  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed());
  base::RunLoop().RunUntilIdle();

  // Warmup probe failed on the insecure proxy for the second network ID.
  std::string second_network_id("test2");
  network_properties_manager.OnChangeInNetworkID(second_network_id);
  // State should be reset when there is a change in the network ID.
  EXPECT_FALSE(network_properties_manager.IsCaptivePortal());
  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed());
  network_properties_manager.SetHasWarmupURLProbeFailed(false, true);
  EXPECT_FALSE(network_properties_manager.IsCaptivePortal());
  EXPECT_FALSE(network_properties_manager.IsInsecureProxyAllowed());
  base::RunLoop().RunUntilIdle();

  // Change back to |first_network_id|.
  network_properties_manager.OnChangeInNetworkID(first_network_id);
  EXPECT_TRUE(network_properties_manager.IsCaptivePortal());
  EXPECT_TRUE(network_properties_manager.IsInsecureProxyAllowed());
  network_properties_manager.SetHasWarmupURLProbeFailed(false, true);

  // Change to |first_network_id|.
  network_properties_manager.OnChangeInNetworkID(first_network_id);
  EXPECT_TRUE(network_properties_manager.IsCaptivePortal());
  EXPECT_FALSE(network_properties_manager.IsInsecureProxyAllowed());

  // Verify the prefs.
  TestNetworkPropertiesManager::ParsedPrefs parsed_prefs =
      network_properties_manager.ForceReadPrefsForTesting();
  ASSERT_EQ(2u, parsed_prefs.size());
  EXPECT_EQ(parsed_prefs.end(), parsed_prefs.find("mismatched_network_id"));
  EXPECT_NE(parsed_prefs.end(), parsed_prefs.find(first_network_id));
  EXPECT_NE(parsed_prefs.end(), parsed_prefs.find(second_network_id));
}

}  // namespace

}  // namespace data_reduction_proxy