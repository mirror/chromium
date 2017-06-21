// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CONFLICTS_ENUMERATE_INPUT_METHOD_EDITORS_WIN_H_
#define CHROME_BROWSER_CONFLICTS_ENUMERATE_INPUT_METHOD_EDITORS_WIN_H_

#include "base/callback_forward.h"

namespace base {
class FilePath;
}

// The path to the registry key where IMEs are registered.
extern const wchar_t kImeRegistryKey[];
extern const wchar_t kClassIdRegistryKeyFormat[];

// Finds third-party IMEs (Input Method Editor) installed on the computer by
// enumerating the registry.
using OnImeEnumeratedCallback =
    base::Callback<void(const base::FilePath&, uint32_t, uint32_t)>;
void EnumerateInputMethodEditors(const OnImeEnumeratedCallback& callback);

#endif  // CHROME_BROWSER_CONFLICTS_ENUMERATE_INPUT_METHOD_EDITORS_WIN_H_
