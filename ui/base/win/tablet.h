// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_WIN_TABLET_H_
#define UI_BASE_WIN_TABLET_H_

#include <stdint.h>
#include <windows.h>

#include <string>
#include <vector>

#include "base/strings/string16.h"
#include "ui/base/ui_base_export.h"

namespace ui {

// A tablet is a device that is touch enabled and also is being used
// "like a tablet". This is used by the following:-
// 1. Metrics:- To gain insight into how users use Chrome.
// 2. Physical keyboard presence :- If a device is in tablet mode, it means
//    that there is no physical keyboard attached.
// 3. To set the right interactions media queries,
//    see https://drafts.csswg.org/mediaqueries-4/#mf-interaction
// This function optionally sets the |reason| parameter to determine as to why
// or why not a device was deemed to be a tablet.
// Returns true if the device is in tablet mode.
UI_BASE_EXPORT bool IsTabletDevice(std::string* reason);

// A slate is a touch device that may have a keyboard attached. This function
// returns true if a keyboard is attached and optionally will set the |reason|
// parameter to the detection method that was used to detect the keyboard.
UI_BASE_EXPORT bool IsKeyboardPresentOnSlate(std::string* reason);

}  // namespace ui

#endif  // UI_BASE_WIN_TABLET_H_
