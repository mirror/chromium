// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_CERT_KNOWN_ROOTS_H_
#define NET_CERT_KNOWN_ROOTS_H_

#include <cstdint>

#include "build/build_config.h"
#include "net/base/net_export.h"

namespace net {

class HashValue;

// Returns a value within the NetRootCert histogram enum indicating the
// ID of the trust anchor whose subjectPublicKeyInfo hash is |hash|, or
// 0 if it cannot be found.
NET_EXPORT int32_t GetNetRootCertHistogramIdForHashValue(const HashValue& hash);

}  // namespace net

#endif  // NET_CERT_KNOWN_ROOTS_H_
