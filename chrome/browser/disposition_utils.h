// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DISPOSITION_UTILS_H_
#define CHROME_BROWSER_DISPOSITION_UTILS_H_

enum class WindowOpenDisposition;

namespace content {
class WebContents;
}

namespace chrome {

// Get/Set the disposition used for a navigation in the WebContents.
WindowOpenDisposition GetDisposition(content::WebContents* tab);
void SetDisposition(content::WebContents* tab,
                    WindowOpenDisposition disposition);

}  // namespace chrome

#endif  // CHROME_BROWSER_DISPOSITION_UTILS_H_
