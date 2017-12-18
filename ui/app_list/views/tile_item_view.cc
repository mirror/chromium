// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/app_list/views/tile_item_view.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "ui/app_list/app_list_constants.h"
#include "ui/app_list/app_list_features.h"
#include "ui/app_list/views/app_list_main_view.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/image/canvas_image_source.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/views/background.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"

namespace {

constexpr int kTopPadding = 5;
constexpr int kIconTitleSpacing = 6;

constexpr int kIconSelectedSize = 56;
constexpr int kIconSelectedCornerRadius = 4;
// Icon selected color, #000 8%.
constexpr int kIconSelectedColor = SkColorSetARGBMacro(0x14, 0x00, 0x00, 0x00);

}  // namespace

namespace app_list {

TileItemView::TileItemView() = default;

TileItemView::~TileItemView() = default;

void TileItemView::Layout() {
  gfx::Rect rect(GetContentsBounds());

  rect.Inset(0, kTopPadding, 0, 0);
  icon_->SetBoundsRect(rect);

  rect.Inset(0, kGridIconDimension + kIconTitleSpacing, 0, 0);
  rect.set_height(title_->GetPreferredSize().height());
  title_->SetBoundsRect(rect);
}

}  // namespace app_list
