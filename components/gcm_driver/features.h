// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_GCM_DRIVER_FEATURES_H
#define COMPONENTS_GCM_DRIVER_FEATURES_H

#include "base/feature_list.h"
#include "components/gcm_driver/common/gcm_driver_export.h"
namespace gcm {
namespace features {
// The period, in days, after which, the GCM token becomes stale. A value of 0
// means it's always fresh.
GCM_DRIVER_EXPORT extern const base::Feature kTokenInvalidationPeriodDays;
// Parameter names
const char kParamNameTokenInvalidationPeriodDays[] =
    "token_invalidation_period";
const int kDefaultTokenInvalidationPeriodDays = 0;
bool IsTokenInvalidationEnabled();
}  // namespace features
}  // namespace gcm
#endif  // COMPONENTS_GCM_DRIVER_FEATURES_H
