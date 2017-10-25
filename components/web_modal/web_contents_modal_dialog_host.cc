// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/web_modal/web_contents_modal_dialog_host.h"
#include "ui/gfx/geometry/size.h"

namespace web_modal {

WebContentsModalDialogHost::~WebContentsModalDialogHost() {
}

gfx::Size WebContentsModalDialogHost::GetMaximumDialogSize(
    const gfx::Size& dialog_preferred_size) {
  return GetMaximumDialogSize();
};

}  // namespace web_modal
