// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/exclusive_access/fullscreen_within_tab_helper.h"

#include "base/macros.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(FullscreenWithinTabHelper);

FullscreenWithinTabHelper::FullscreenWithinTabHelper(
    content::WebContents* ignored)
#if defined(OS_MACOSX)
    : is_fullscreen_or_pending_(false) {
}
#else
    : is_fullscreen_for_captured_tab_(false) {
}
#endif

FullscreenWithinTabHelper::~FullscreenWithinTabHelper() {}

// static
#if defined(OS_MACOSX)
bool FullscreenWithinTabHelper::IsContentFullscreenEnabled() {
  return base::FeatureList::IsEnabled(features::kContentFullscreen);
}
#endif

// static
void FullscreenWithinTabHelper::RemoveForWebContents(
    content::WebContents* web_contents) {
  DCHECK(web_contents);
  web_contents->RemoveUserData(UserDataKey());
}
