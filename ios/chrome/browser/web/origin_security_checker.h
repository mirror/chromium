// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_WEB_ORIGIN_SECURITY_CHECKER_H_
#define IOS_CHROME_BROWSER_WEB_ORIGIN_SECURITY_CHECKER_H_

class GURL;

namespace web {

// Returns true for a valid |url| which is a secure origin and doesn't have
// data URI scheme.
bool IsOriginSecureAndNonData(const GURL& url);

}  // namespace web

#endif  // IOS_CHROME_BROWSER_WEB_ORIGIN_SECURITY_CHECKER_H_
