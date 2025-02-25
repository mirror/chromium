// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/dbus/finch_features_service_provider_delegate.h"

#include "base/feature_list.h"
#include "chrome/common/chrome_features.h"

namespace chromeos {

FinchFeaturesServiceProviderDelegate::FinchFeaturesServiceProviderDelegate() {}

FinchFeaturesServiceProviderDelegate::~FinchFeaturesServiceProviderDelegate() {}

bool FinchFeaturesServiceProviderDelegate::IsCrostiniEnabled() {
  return base::FeatureList::IsEnabled(features::kCrostini);
}

}  // namespace chromeos
