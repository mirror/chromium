// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_METRICS_METRICS_FEATURES_H_
#define BASE_METRICS_METRICS_FEATURES_H_

#include "base/feature_list.h"

namespace base {
namespace metrics {

// Whether the user is a Chrome dogfooder.
BASE_EXPORT bool IsDogFoodUser();

}  // namespace metrics
}  // namespace base

#endif  // BASE_METRICS_METRICS_FEATURES_H_
