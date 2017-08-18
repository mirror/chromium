// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/shell_dialogs/select_file_dialog.h"

namespace ui {

SelectFileDialog* CreateSelectFileDialog(SelectFileDialog::Listener* listener,
                                         SelectFilePolicy* policy) {
  // TODO(fuchsia): https://crbug.com/746674
  NOTIMPLEMENTED();
  return nullptr;
}

}  // namespace ui
