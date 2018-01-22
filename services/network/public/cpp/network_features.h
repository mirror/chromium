// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_PUBLIC_CPP_NETWORK_FEATURES_
#define SERVICES_NETWORK_PUBLIC_CPP_NETWORK_FEATURES_

#include "base/feature_list.h"
#include "services/network/public/cpp/network_features_export.h"

namespace features {

NETWORK_FEATURES_EXPORT extern const base::Feature kReporting;
NETWORK_FEATURES_EXPORT extern const base::Feature kNetworkErrorLogging;

}  // namespace features

#endif  // SERVICES_NETWORK_PUBLIC_CPP_NETWORK_FEATURES_
