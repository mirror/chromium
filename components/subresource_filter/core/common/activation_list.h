// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SUBRESOURCE_FILTER_CORE_COMMON_ACTIVATION_LIST_H_
#define COMPONENTS_SUBRESOURCE_FILTER_CORE_COMMON_ACTIVATION_LIST_H_

#include <iosfwd>

namespace subresource_filter {

enum class ActivationList {
  NONE,
  SOCIAL_ENG_ADS_INTERSTITIAL,
  PHISHING_INTERSTITIAL,
  SUBRESOURCE_FILTER,

  // Site violates the better ads standard.
  BETTER_ADS,

  // Site shows abusive ads.
  ABUSIVE_ADS,

  // Site violates the better ads standard and shows abusive ads.
  ALL_ADS,
  LAST = ALL_ADS
};

// For logging use only.
std::ostream& operator<<(std::ostream& os, const ActivationList& type);

}  // namespace subresource_filter

#endif  // COMPONENTS_SUBRESOURCE_FILTER_CORE_COMMON_ACTIVATION_LIST_H_
