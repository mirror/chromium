// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/metrics/metrics_features.h"

#include "base/feature_list.h"

namespace base {
namespace metrics {

namespace {

// Enabled for Chrome dogfooders.
const Feature kDogfood{"Dogfood", FEATURE_DISABLED_BY_DEFAULT};

}  // namespace

bool IsDogfoodUser() {
  return FeatureList::IsEnabled(kDogfood);
}

}  // namespace metrics
}  // namespace base
