// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_EXTENSIONS_EXTENSION_NAME_H_
#define CHROME_BROWSER_UI_EXTENSIONS_EXTENSION_NAME_H_

#include "url/gurl.h"

namespace content {
class WebContents;
}  // namespace content

namespace extensions {

// If |url| is an extension URL, returns the name of the associated extension,
// with whitespace collapsed. Otherwise, returns empty string. |web_contents|
// is used to get at the extension registry.
base::string16 GetExtensionName(const GURL& url,
                                content::WebContents* web_contents);

}  // namespace extensions

#endif  // CHROME_BROWSER_UI_EXTENSIONS_EXTENSION_NAME_H_
