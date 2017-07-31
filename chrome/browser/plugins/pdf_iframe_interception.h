// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PLUGINS_PDF_IFRAME_INTERCEPTION_H_
#define CHROME_BROWSER_PLUGINS_PDF_IFRAME_INTERCEPTION_H_

#include <memory>

#include "base/macros.h"

namespace content {
class NavigationHandle;
class NavigationThrottle;
}  // namespace content

class PDFIFrameInterception {
 public:
  static std::unique_ptr<content::NavigationThrottle> MaybeCreateThrottleFor(
      content::NavigationHandle* handle);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(PDFIFrameInterception);
};

#endif  // CHROME_BROWSER_PLUGINS_PDF_IFRAME_INTERCEPTION_H_
