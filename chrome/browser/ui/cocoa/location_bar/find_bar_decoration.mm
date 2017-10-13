// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/location_bar/find_bar_decoration.h"

#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/command_updater.h"
#import "chrome/browser/ui/cocoa/omnibox/omnibox_view_mac.h"
#import "chrome/browser/ui/cocoa/themed_window.h"
#include "chrome/grit/generated_resources.h"
#include "components/strings/grit/components_strings.h"
#include "components/toolbar/vector_icons.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/gfx/color_palette.h"

FindBarDecoration::FindBarDecoration() {
  SetVisible(true);
}

FindBarDecoration::~FindBarDecoration() {}

bool FindBarDecoration::AcceptsMousePress() {
  return false;
}

NSString* FindBarDecoration::GetToolTip() {
  return tooltip_.get();
}

SkColor FindBarDecoration::GetMaterialIconColor(
    bool location_bar_is_dark) const {
  return gfx::kChromeIconGrey;
}

const gfx::VectorIcon* FindBarDecoration::GetMaterialVectorIcon() const {
  return &toolbar::kFindInPageIcon;
}
