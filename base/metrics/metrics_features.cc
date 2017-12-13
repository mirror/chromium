// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/metrics/metrics_features.h"

namespace base {
namespace metrics {

namespace {

// Enabled for Chrome dog fooders.
const base::Feature kDogfood{"Dogfood", base::FEATURE_DISABLED_BY_DEFAULT};

}  // namespace

bool IsDogFoodUser() {
  return base::FeatureList::IsEnabled(kDogfood);
}

}  // namespace metrics
}  // namespace base
