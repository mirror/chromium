// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_IMPORTER_IMPORT_LOCK_DIALOG_COCOA_H_
#define CHROME_BROWSER_UI_COCOA_IMPORTER_IMPORT_LOCK_DIALOG_COCOA_H_

#include "base/callback_forward.h"
#include "ui/gfx/native_widget_types.h"

namespace importer {

void ShowImportLockDialogCocoa(gfx::NativeWindow parent,
                               const base::Callback<void(bool)>& callback);

}  // namespace importer

#endif  // CHROME_BROWSER_UI_COCOA_IMPORTER_IMPORT_LOCK_DIALOG_COCOA_H_
