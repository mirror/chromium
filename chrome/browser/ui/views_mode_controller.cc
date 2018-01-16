// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views_mode_controller.h"

#include "build/build_config.h"
#include "chrome/common/chrome_features.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/ui_features.h"

namespace {
#if defined(OS_MACOSX) && !BUILDFLAG(MAC_VIEWS_BROWSER)
constexpr bool kForceViews = false;
#else
constexpr bool kForceViews = true;
#endif
}

namespace views_mode_controller {

bool IsBrowserViews() {
  return kForceViews || base::FeatureList::IsEnabled(features::kViewsBrowser);
}

bool ArePilotDialogsViews() {
  return kForceViews || ui::MaterialDesignController::IsSecondaryUiMaterial();
}

bool AreAllDialogsViews() {
  return kForceViews || (ArePilotDialogsViews() &&
                         base::FeatureList::IsEnabled(
                             features::kShowAllDialogsWithViewsToolkit));
}

}  // namespace views_mode_controller
