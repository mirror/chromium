// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/bookmark_app_util.h"

#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/features.h"

namespace extensions {

bool IsExperimentalBookmarkAppBrowser(const Browser* browser) {
  // All app browsers (i.e bookmark apps and hosted apps) except devtools should
  // be given the new treatment.
  return base::FeatureList::IsEnabled(features::kDesktopPWAWindowing) &&
         browser->is_app() && !browser->is_devtools();
}

}  // namespace extensions
