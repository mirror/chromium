// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/chrome_cleaner/chrome_cleaner_reboot_dialog_controller_impl_win.h"

namespace safe_browsing {

ChromeCleanerRebootDialogControllerImpl::
    ChromeCleanerRebootDialogControllerImpl() {}

void ChromeCleanerRebootDialogControllerImpl::DialogShown() {}

void ChromeCleanerRebootDialogControllerImpl::Accept() {
  // InitiateReboot();
}

void ChromeCleanerRebootDialogControllerImpl::Cancel() {}

void ChromeCleanerRebootDialogControllerImpl::Close() {}

ChromeCleanerRebootDialogControllerImpl::
    ~ChromeCleanerRebootDialogControllerImpl() {}

void ChromeCleanerRebootDialogControllerImpl::OnInteractionDone() {
  delete this;
}

}  // namespace safe_browsing
