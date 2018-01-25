// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_PUBLIC_CPP_NETWORK_SWITCHES_H_
#define SERVICES_NETWORK_PUBLIC_CPP_NETWORK_SWITCHES_H_

#include "services/network/public/cpp/export.h"

namespace network {

namespace switches {

SERVICES_NETWORK_EXPORT extern const char kHostResolverRules[];
SERVICES_NETWORK_EXPORT extern const char kLogNetLog[];
SERVICES_NETWORK_EXPORT extern const char kNoReferrers[];

}  // namespace switches

}  // namespace network

#endif  // SERVICES_NETWORK_PUBLIC_CPP_NETWORK_SWITCHES_H_
