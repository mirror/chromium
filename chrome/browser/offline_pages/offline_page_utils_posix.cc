// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/offline_pages/offline_page_utils.h"

#include "base/logging.h"
#include "content/public/browser/web_contents.h"

// Posix-specific part of OfflinePageUtils.
// TODO(romax): Most of the functionalities are not implemented. This is meant
// to get the build working first. Try implement these functions.

namespace offline_pages {

// static
bool OfflinePageUtils::GetTabId(content::WebContents* web_contents,
                                int* tab_id) {
  return false;
}

// static
bool OfflinePageUtils::CurrentlyShownInCustomTab(
    content::WebContents* web_contents) {
  return false;
}

// static
void OfflinePageUtils::ShowDuplicatePrompt(
    const base::Closure& confirm_continuation,
    const GURL& url,
    bool exists_duplicate_request,
    content::WebContents* web_contents) {
  // NOT IMPLEMENTED.
}

// static
void OfflinePageUtils::ShowDownloadingToast() {
  // NOT IMPLEMENTED.
}

}  // namespace offline_pages
