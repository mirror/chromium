// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/metrics/field_trial.h"
#include "base/metrics/field_trial_param_associator.h"
#include "base/metrics/field_trial_params.h"
#include "base/test/scoped_feature_list.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "url/gurl.h"

namespace content {

class ResourceSchedulerBrowserTest : public ContentBrowserTest {
 protected:
  ResourceSchedulerBrowserTest() : field_trial_list_(nullptr) {}

  void InitializeMaxDelayableRequestsExperiment(
      base::test::ScopedFeatureList* scoped_feature_list,
      bool enabled) {
    base::FieldTrialParamAssociator::GetInstance()->ClearAllParamsForTesting();
    const char kTrialName[] = "TrialName";
    const char kGroupName[] = "GroupName";
    const char kMaxDelayableRequestsNetworkOverride[] =
        "MaxDelayableRequestsNetworkOverride";

    std::map<std::string, std::string> params;
    params["MaxEffectiveConnectionType"] = "2G";
    params["MaxBDPKbits1"] = "130";
    params["MaxDelayableRequests1"] = "2";
    params["MaxBDPKbits2"] = "160";
    params["MaxDelayableRequests2"] = "4";

    base::AssociateFieldTrialParams(kTrialName, kGroupName, params);
    base::FieldTrial* field_trial =
        base::FieldTrialList::CreateFieldTrial(kTrialName, kGroupName);

    std::unique_ptr<base::FeatureList> feature_list(new base::FeatureList);
    feature_list->RegisterFieldTrialOverride(
        kMaxDelayableRequestsNetworkOverride,
        enabled ? base::FeatureList::OVERRIDE_ENABLE_FEATURE
                : base::FeatureList::OVERRIDE_DISABLE_FEATURE,
        field_trial);
    scoped_feature_list->InitWithFeatureList(std::move(feature_list));
  }

 private:
  base::FieldTrialList field_trial_list_;
};

IN_PROC_BROWSER_TEST_F(ResourceSchedulerBrowserTest,
                       ResourceLoadingExperimentIncognito) {
  ASSERT_TRUE(embedded_test_server()->Start());
  GURL url(embedded_test_server()->GetURL(
      "/resource_loading/resource_loading_non_mobile.html"));
  for (auto experiment_status : {true, false}) {
    base::test::ScopedFeatureList feature_list;
    InitializeMaxDelayableRequestsExperiment(&feature_list, experiment_status);
    Shell* otr = CreateOffTheRecordBrowser();
    NavigateToURL(otr, url);
    int data = -1;
    EXPECT_TRUE(ExecuteScriptAndExtractInt(otr, "getResourceNumber()", &data));
    EXPECT_EQ(9, data);
  }
}

IN_PROC_BROWSER_TEST_F(ResourceSchedulerBrowserTest,
                       ResourceLoadingExperimentNormal) {
  ASSERT_TRUE(embedded_test_server()->Start());
  GURL url(embedded_test_server()->GetURL(
      "/resource_loading/resource_loading_non_mobile.html"));
  for (auto experiment_status : {true, false}) {
    base::test::ScopedFeatureList feature_list;
    InitializeMaxDelayableRequestsExperiment(&feature_list, experiment_status);
    Shell* otr = shell();
    NavigateToURL(otr, url);
    int data = -1;
    EXPECT_TRUE(ExecuteScriptAndExtractInt(otr, "getResourceNumber()", &data));
    EXPECT_EQ(9, data);
  }
}

}  // namespace content
