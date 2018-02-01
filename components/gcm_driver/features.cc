// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/gcm_driver/features.h"
#include "base/feature_list.h"
#include "components/gcm_driver/common/gcm_driver_export.h"

namespace gcm {
namespace features {
const base::Feature kTokenInvalidationPeriodDays{
    "TokenInvalidAfterDays, base::FEATURE_ENABLED_BY_DEFAULT"};
bool IsTokenInvalidationEnabled() {
  return base::FeatureList::IsEnabled(kTokenInvalidationPeriodDays);
}
}  // namespace features
}  // namespace gcm
