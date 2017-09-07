// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/blocked_content/popup_tracker.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "content/public/browser/web_contents.h"

const void* const PopupTracker::kUserDataKey = &PopupTracker::kUserDataKey;

// static
void PopupTracker::CreateForWebContents(content::WebContents* web_contents) {
  DCHECK(!web_contents->GetUserData(kUserDataKey));
  web_contents->SetUserData(kUserDataKey,
                            base::WrapUnique(new PopupTracker(web_contents)));
}

PopupTracker::~PopupTracker() = default;

PopupTracker::PopupTracker(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {}
