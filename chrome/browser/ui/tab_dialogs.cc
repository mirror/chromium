// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/tab_dialogs.h"
#include "content/public/browser/web_contents.h"

namespace {
int kUserDataKey;  // The value is not important, the address is a key.
}

// static
TabDialogs* TabDialogs::g_tab_dialogs_for_test_ = nullptr;

// static
TabDialogs* TabDialogs::FromWebContents(content::WebContents* contents) {
  DCHECK(contents);
  if (g_tab_dialogs_for_test_)
    return g_tab_dialogs_for_test_;
  return static_cast<TabDialogs*>(contents->GetUserData(UserDataKey()));
}

// static
const void* TabDialogs::UserDataKey() {
  return &kUserDataKey;
}

// static
void TabDialogs::SetTabDialogsForTest(TabDialogs* tab_dialogs) {
  g_tab_dialogs_for_test_ = tab_dialogs;
}
