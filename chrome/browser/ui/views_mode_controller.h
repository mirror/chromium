// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_MODE_CONTROLLER_H_
#define CHROME_BROWSER_UI_VIEWS_MODE_CONTROLLER_H_

#include "build/build_config.h"
#include "build/buildflag.h"
#include "ui/base/ui_features.h"

#if defined(OS_MACOSX) && BUILDFLAG(MAC_VIEWS_BROWSER)

namespace views_mode_controller {

// This function returns whether a Views browser build should use the Cocoa UI.
// This function is only available in mac_views_browser = true builds, which
// link both the Views browser UI and the Cocoa browser UI; it is used to choose
// between them. See https://crbug.com/802257.
bool IsViewsBrowserCocoa();

}  // namespace views_mode_controller

#endif

#endif  // CHROME_BROWSER_UI_VIEWS_MODE_CONTROLLER_H_
