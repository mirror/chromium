// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/subresource_filter/subresource_filter_failsafe.h"

#include <utility>
#include <vector>

#include "base/feature_list.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search/search.h"
#include "components/subresource_filter/core/browser/subresource_filter_features.h"
#include "url/gurl.h"

// Control the class as a feature to allow command line experimentation without
// accidentally triggering the failsafe.
const base::Feature kSubresourceFilterFailsafe{
    "SubresourceFilterFailsafe", base::FEATURE_ENABLED_BY_DEFAULT};

SubresourceFilterFailsafe::SubresourceFilterFailsafe(Profile* profile)
    : profile_(profile) {}
SubresourceFilterFailsafe::~SubresourceFilterFailsafe() = default;

void SubresourceFilterFailsafe::OnDidShowUI(
    const GURL& url,
    const subresource_filter::Configuration& matching_config) {
  if (!base::FeatureList::IsEnabled(kSubresourceFilterFailsafe))
    return;

  // TODO(csharrison): DCHECK that this is not the forced activation
  // configuration.

  // TODO(csharrison): Add more triggers.
  bool is_ntp = search::IsInstantNTPURL(url, profile_);
  if (is_ntp)
    RemoveConfiguration(matching_config);
}

void SubresourceFilterFailsafe::RemoveConfiguration(
    const subresource_filter::Configuration& config) {
  const std::vector<subresource_filter::Configuration>&
      configs_by_decreasing_priority(
          subresource_filter::GetEnabledConfigurations()
              ->configs_by_decreasing_priority());
  std::vector<subresource_filter::Configuration> new_configs;
  for (const subresource_filter::Configuration& c :
       configs_by_decreasing_priority) {
    if (c != config)
      new_configs.push_back(c);
  }
  // There must be a match.
  DCHECK_NE(new_configs.size(), configs_by_decreasing_priority.size());
  GetAndSetActivateConfigurations(
      base::MakeRefCounted<subresource_filter::ConfigurationList>(
          std::move(new_configs)));
}
