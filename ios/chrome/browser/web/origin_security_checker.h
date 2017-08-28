// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_WEB_ORIGIN_SECURITY_CHECKER_H_
#define IOS_CHROME_BROWSER_WEB_ORIGIN_SECURITY_CHECKER_H_

#include "base/macros.h"
#include "components/security_state/core/security_state.h"

class GURL;

namespace web {

// Returns true for a valid |url| from a secure context. This check is a
// superset of the web::IsOriginSecure() and url.is_valid() by making
// sure the URL does not have a data URI scheme. This may eventually get more
// complicated and require looking at whether the origin is opaque, etc.
bool IsContextSecure(const GURL& url);

}  // namespace web

#endif  // IOS_CHROME_BROWSER_WEB_ORIGIN_SECURITY_CHECKER_H_
