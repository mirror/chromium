// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/network_error_logging/network_error_logging_feature.h"

#include "base/feature_list.h"

namespace features {

const base::Feature kNetworkErrorLogging{"NetworkErrorLogging",
                                         base::FEATURE_DISABLED_BY_DEFAULT};

}  // namespace features
