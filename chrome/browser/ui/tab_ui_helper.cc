// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/tab_ui_helper.h"

#include "chrome/browser/sessions/session_restore.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(TabUIHelper);

TabUIHelper::TabUIHelper(content::WebContents* contents)
    : WebContentsObserver(contents) {
  SessionRestore::AddObserver(this);
}

TabUIHelper::~TabUIHelper() {
  SessionRestore::RemoveObserver(this);
}

void TabUIHelper::OnWillRestoreTab(content::WebContents* contents) {
  created_by_session_restore_ = true;
}

void TabUIHelper::DidStopLoading() {
  // Reset the properties after the initial navigation finishes loading, so that
  // latter navigations are not affected.
  is_navigation_delayed_ = false;
  created_by_session_restore_ = false;
  title_ = base::UTF8ToUTF16("");
}
