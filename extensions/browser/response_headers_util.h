// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_RESPONSE_HEADERS_UTIL_H_
#define EXTENSIONS_BROWSER_RESPONSE_HEADERS_UTIL_H_

#include <string>

class GURL;

namespace extensions {

// Returns true if the header shall be hidden to extensions.
bool HideResponseHeader(const GURL& url, const std::string& header_name);

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_RESPONSE_HEADERS_UTIL_H_
