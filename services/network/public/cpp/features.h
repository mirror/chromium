// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_PUBLIC_CPP_FEATURES_H_
#define SERVICES_NETWORK_PUBLIC_CPP_FEATURES_H_

#include "base/feature_list.h"
#include "services/network/public/cpp/export.h"

namespace network {
namespace features {

SERVICES_NETWORK_EXPORT extern const base::Feature kNetworkService;
SERVICES_NETWORK_EXPORT extern const base::Feature kOutOfBlinkCORS;
SERVICES_NETWORK_EXPORT extern const base::Feature
    kRendererSideResourceScheduler;

}  // namespace features
}  // namespace network

#endif  // SERVICES_NETWORK_PUBLIC_CPP_FEATURES_H_
