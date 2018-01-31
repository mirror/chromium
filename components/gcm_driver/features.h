// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_GCM_DRIVER_FEATURES_H
#define COMPONENTS_GCM_DRIVER_FEATURES_H

#include "base/feature_list.h"
namespace gcm {
// The period, in days, after which, the GCM token becomes stale. A value of 0
// means it's always fresh.
extern const base::Feature kTokenInvalidationPeriodDays;
extern bool IsTokenValidationEnabled();
}  // namespace gcm
#endif  // COMPONENTS_GCM_DRIVER_FEATURES_H
