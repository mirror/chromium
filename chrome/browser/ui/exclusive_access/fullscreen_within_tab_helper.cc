// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/exclusive_access/fullscreen_within_tab_helper.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(FullscreenWithinTabHelper);

FullscreenWithinTabHelper::FullscreenWithinTabHelper(
    content::WebContents* ignored)
    :
#if defined(OS_MACOSX)
      is_fullscreen_or_pending_(false),
      separate_fullscreen_window_(nullptr),
#endif
      is_fullscreen_for_captured_tab_(false) {
}

FullscreenWithinTabHelper::~FullscreenWithinTabHelper() {}

// static
void FullscreenWithinTabHelper::RemoveForWebContents(
    content::WebContents* web_contents) {
  DCHECK(web_contents);
  web_contents->RemoveUserData(UserDataKey());
}
