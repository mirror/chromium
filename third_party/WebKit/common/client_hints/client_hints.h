// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_COMMON_CLIENT_HINTS_CLIENT_HINTS_H_
#define THIRD_PARTY_WEBKIT_COMMON_CLIENT_HINTS_CLIENT_HINTS_H_

#include <stddef.h>

#include "third_party/WebKit/common/common_export.h"

namespace blink {

// Mapping from WebClientHintsType to the header value for enabling the
// corresponding client hint. The ordering should match the ordering of enums in
// third_party/WebKit/public/platform/WebClientHintsType.h.
extern const char* const BLINK_COMMON_EXPORT kClientHintsHeaderMapping[];

extern const size_t BLINK_COMMON_EXPORT kClientHintsHeaderMappingCount;

}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_COMMON_CLIENT_HINTS_CLIENT_HINTS_H_
