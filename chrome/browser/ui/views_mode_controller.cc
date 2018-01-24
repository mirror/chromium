// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views_mode_controller.h"

#if defined(OS_MACOSX) && BUILDFLAG(MAC_VIEWS_BROWSER)

#include "base/command_line.h"

namespace views_mode_controller {

static constexpr char kPolyChrome[] = "polychrome";

bool IsViewsBrowserCocoa() {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(kPolyChrome);
}

}  // namespace views_mode_controller

#endif  // OS_MACOSX
