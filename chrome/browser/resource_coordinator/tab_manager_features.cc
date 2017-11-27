// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/tab_manager_features.h"

#include "base/metrics/field_trial_params.h"
#include "chrome/common/chrome_features.h"

namespace {

constexpr char kTabLoadTimeoutInMsParameterName[] = "tabLoadTimeoutInMs";

}  // namespace

namespace features {

// Enables using customized value for tab load timeout. This is used by both
// staggered background tab opening and session restore in finch experiment to
// see what timeout value is better. The default timeout is used when this
// feature is disabled.
const base::Feature kCustomizedTabLoadTimeout{
    "CustomizedTabLoadTimeout", base::FEATURE_DISABLED_BY_DEFAULT};

// Enables proactive tab discarding.
const base::Feature kProactiveTabDiscarding{"ProactiveTabDiscarding",
                                            base::FEATURE_DISABLED_BY_DEFAULT};

// Enables delaying the navigation of background tabs in order to improve
// foreground tab's user experience.
const base::Feature kStaggeredBackgroundTabOpening{
    "StaggeredBackgroundTabOpening", base::FEATURE_DISABLED_BY_DEFAULT};

// This controls whether we are running experiment with staggered background
// tab opening feature. For control group, this should be disabled. This depends
// on |kStaggeredBackgroundTabOpening| above.
const base::Feature kStaggeredBackgroundTabOpeningExperiment{
    "StaggeredBackgroundTabOpeningExperiment",
    base::FEATURE_ENABLED_BY_DEFAULT};

}  // namespace features

namespace resource_coordinator {

// Field-trial parameter names for proactive tab discarding.
constexpr char kProactiveTabDiscard_MaxLoadedTabsPerGbRamParam[] =
    "MaxLoadedTabsPerGbRam";
constexpr char kProactiveTabDiscard_MinLoadedTabCountParam[] =
    "MinLoadedTabCount";
constexpr char kProactiveTabDiscard_MaxLoadedTabCountParam[] =
    "MaxLoadedTabCount";
constexpr char kProactiveTabDiscard_MinOccludedTimeoutParam[] =
    "MinOccludedTimeoutSeconds";
constexpr char kProactiveTabDiscard_MaxOccludedTimeoutParam[] =
    "MaxOccludedTimeoutSeconds";

// Default values for ProactiveTabDiscardParams.
//
// Testing in the lab shows that 2GB devices suffer beyond 6 tabs, and 4GB
// devices suffer beyond about 12 tabs. As a very simple first step, we'll aim
// at allowing 3 tabs per GB of RAM on a system before proactive discarding
// kicks in. This is a system resource dependent max, which is combined with the
// DefaultMaxLoadedTabCount to determine the max on a system.
constexpr uint32_t kProactiveTabDiscard_MaxLoadedTabsPerGbRamDefault = 3;
// 50% of people cap out at 4 tabs, so for them proactive discarding won't even
// be invoked. See Tabs.MaxTabsInADay.
// TODO(chrisha): This should eventually be informed by the number of tabs
// typically used over a given time horizon (metric being developed).
constexpr uint32_t kProactiveTabDiscard_MinLoadedTabCountDefault = 4;
// 99.9% of people cap out with fewer than this number, so only 0.1% of the
// population should ever encounter proactive discarding based on this cap.
constexpr uint32_t kProactiveTabDiscard_MaxLoadedTabCountDefault = 100;
// How long a tab needs to be occluded before it is eligible for proactive
// discarding. Beyond this it will be protected. Note that this does not mean it
// will automatically be proactively discarded beyond this point, just that it
// will be eligible for proactive discarding based on tab counts.
constexpr base::TimeDelta kProactiveTabDiscard_MinOccludedTimeoutDefault =
    base::TimeDelta::FromMinutes(10);
// The amount of time beyond which a tab is always proactively discarded if
// possible.
constexpr base::TimeDelta kProactiveTabDiscard_MaxOccludedTimeoutDefault =
    base::TimeDelta::FromHours(12);

void GetProactiveTabDiscardParams(ProactiveTabDiscardParams* params) {
  params->max_loaded_tabs_per_gb_ram = base::GetFieldTrialParamByFeatureAsInt(
      features::kProactiveTabDiscarding,
      kProactiveTabDiscard_MaxLoadedTabsPerGbRamParam,
      kProactiveTabDiscard_MaxLoadedTabsPerGbRamDefault);

  params->min_loaded_tab_count = base::GetFieldTrialParamByFeatureAsInt(
      features::kProactiveTabDiscarding,
      kProactiveTabDiscard_MinLoadedTabCountParam,
      kProactiveTabDiscard_MinLoadedTabCountDefault);

  params->max_loaded_tab_count = base::GetFieldTrialParamByFeatureAsInt(
      features::kProactiveTabDiscarding,
      kProactiveTabDiscard_MaxLoadedTabCountParam,
      kProactiveTabDiscard_MaxLoadedTabCountDefault);

  params->min_occluded_timeout =
      base::TimeDelta::FromSeconds(base::GetFieldTrialParamByFeatureAsInt(
          features::kProactiveTabDiscarding,
          kProactiveTabDiscard_MinOccludedTimeoutParam,
          kProactiveTabDiscard_MinOccludedTimeoutDefault.InSeconds()));

  params->max_occluded_timeout =
      base::TimeDelta::FromSeconds(base::GetFieldTrialParamByFeatureAsInt(
          features::kProactiveTabDiscarding,
          kProactiveTabDiscard_MaxOccludedTimeoutParam,
          kProactiveTabDiscard_MaxOccludedTimeoutDefault.InSeconds()));
}

base::TimeDelta GetTabLoadTimeout(const base::TimeDelta& default_timeout) {
  int timeout_in_ms = base::GetFieldTrialParamByFeatureAsInt(
      features::kCustomizedTabLoadTimeout, kTabLoadTimeoutInMsParameterName,
      default_timeout.InMilliseconds());

  if (timeout_in_ms <= 0)
    return default_timeout;

  return base::TimeDelta::FromMilliseconds(timeout_in_ms);
}

}  // namespace resource_coordinator
