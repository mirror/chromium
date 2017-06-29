// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/importer/importer_lock_dialog.h"
#include "chrome/browser/ui/cocoa/importer/import_lock_dialog_cocoa.h"
#include "chrome/browser/ui/views/importer/import_lock_dialog_view.h"
#include "ui/base/material_design/material_design_controller.h"

namespace importer {

void ShowImportLockDialog(gfx::NativeWindow parent,
                          const base::Callback<void(bool)>& callback) {
  if (ui::MaterialDesignController::IsSecondaryUiMaterial())
    ImportLockDialogView::Show(parent, callback);
  else
    ShowImportLockDialogCocoa(parent, callback);
}

}  // namespace importer
