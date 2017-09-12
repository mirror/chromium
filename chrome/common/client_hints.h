// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_CLIENT_HINTS_H_
#define CHROME_COMMON_CLIENT_HINTS_H_

#include "components/content_settings/core/common/content_settings.h"
#include "third_party/WebKit/public/platform/WebClientHintsType.h"
#include "third_party/WebKit/public/platform/WebURL.h"
#include "url/gurl.h"

namespace client_hints {

// Allow passing both WebURL and GURL here, so that we can early return without
// allocating a new backing string if only the default rule matches.

void GetAllowedClientHintsFromSource(
    const GURL& url,
    const ContentSettingsForOneType& client_hints_rules,
    blink::WebEnabledClientHints* client_hints);

}  // namespace client_hints

#endif  // CHROME_COMMON_CLIENT_HINTS_H_
