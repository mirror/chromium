// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/layout_constants.h"

#include "base/logging.h"
#include "build/build_config.h"
#include "ui/base/material_design/material_design_controller.h"

int GetLayoutConstant(LayoutConstant constant) {
  const bool hybrid = ui::MaterialDesignController::GetMode() ==
                      ui::MaterialDesignController::MATERIAL_HYBRID;
  const bool touch_optimized_material =
      ui::MaterialDesignController::IsTouchOptimizedUiEnabled();

  constexpr int kTabHeight[] = {29, 33, 41};
  const int mode = ui::MaterialDesignController::GetMode();

  switch (constant) {
    case LOCATION_BAR_BUBBLE_VERTICAL_PADDING:
      return hybrid ? 1 : 3;
    case LOCATION_BAR_BUBBLE_FONT_VERTICAL_PADDING:
      return hybrid ? 3 : 2;
    case LOCATION_BAR_BUBBLE_ANCHOR_VERTICAL_INSET:
      if (ui::MaterialDesignController::IsSecondaryUiMaterial())
        return 1;
      return hybrid ? 8 : 6;
    case LOCATION_BAR_ELEMENT_PADDING:
      return hybrid ? 3 : 1;
    case LOCATION_BAR_HEIGHT:
      return hybrid ? 32 : 28;
    case TABSTRIP_NEW_TAB_BUTTON_OVERLAP:
      return hybrid ? 6 : 5;
    case TAB_HEIGHT:
      return kTabHeight[mode];
    case TOOLBAR_ELEMENT_PADDING:
      return hybrid ? 8 : 0;
    case TOOLBAR_STANDARD_SPACING:
      return hybrid ? 8 : 4;
    case TAB_TOUCH_WIDTH:
      return touch_optimized_material ? 68 : 120;
    case TAB_STACK_PADDING:
      return touch_optimized_material ? 4 : 6;
    case TAB_STANDARD_WIDTH:
      return touch_optimized_material ? 245 : 193;
    case TAB_PRE_TITLE_SPACING:
      return touch_optimized_material ? 8 : 6;
    case TAB_AFTER_TITLE_SPACING:
      return touch_optimized_material ? 8 : 4;
    case TAB_ALERT_INDICATOR_ICON_DEFAULT_WIDTH:
      return touch_optimized_material ? 12 : 16;
    case TAB_ALERT_INDICATOR_PRESENTING_ICON_WIDTH:
      return 16;
  }
  NOTREACHED();
  return 0;
}

gfx::Insets GetLayoutInsets(LayoutInset inset) {
  const int mode = ui::MaterialDesignController::GetMode();
  constexpr int kInsetTabHorizontal[] = {16, 18, 24};

  switch (inset) {
    case TAB:
      return gfx::Insets(1, kInsetTabHorizontal[mode]);
  }
  NOTREACHED();
  return gfx::Insets();
}

gfx::Size GetLayoutSize(LayoutSize size) {
  const bool hybrid = ui::MaterialDesignController::GetMode() ==
                      ui::MaterialDesignController::MATERIAL_HYBRID;
  switch (size) {
    case NEW_TAB_BUTTON:
      return hybrid ? gfx::Size(39, 21) : gfx::Size(36, 18);
  }
  NOTREACHED();
  return gfx::Size();
}
