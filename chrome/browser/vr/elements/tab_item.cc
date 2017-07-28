// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/tab_item.h"

#include "base/memory/ptr_util.h"
#include "cc/base/math_util.h"
#include "chrome/browser/vr/elements/tab_item_texture.h"
#include "chrome/browser/vr/elements/tooltip.h"
#include "chrome/browser/vr/target_property.h"
#include "chrome/browser/vr/ui_scene.h"
#include "chrome/browser/vr/ui_scene_manager.h"

namespace vr {

namespace {

float constexpr kTooltipHeight = 0.1f;
float constexpr kTooltipGap = 0.02f;

bool ApproximatelyEqual(float lhs, float rhs, float tolerance) {
  DCHECK_LE(0, tolerance);
  return std::abs(rhs - lhs) <= tolerance;
}

}  // namespace

TabItem::TabItem(UiSceneManager* mgr, UiScene* scene)
    : TexturedElement(512), texture_(base::MakeUnique<TabItemTexture>()) {
  // Add the tooltip.
  auto element = base::MakeUnique<Tooltip>();
  element->set_id(mgr->AllocateId());
  element->set_draw_phase(2);
  element->SetSize(size().width(), kTooltipHeight);
  element->SetOpacity(0.0f);
  element->animation_player().SetTransitionedProperties({OPACITY});
  element->SetBackgroundColor(0xE0EFEFEF);
  element->SetTextColor(0xFF212121);
  element->set_hit_testable(false);
  element->set_y_anchoring(YAnchoring::YBOTTOM);
  element->SetTranslate(0, -kTooltipHeight / 2 - kTooltipGap, 0);
  AddChild(element.get());
  tooltip_ = element.get();
  scene->AddUiElement(std::move(element));
}

TabItem::~TabItem() = default;

void TabItem::SetSize(float width, float height) {
  auto prev_size = size();
  TexturedElement::SetSize(width, height);
  // Currently, we resize textured elements based on the drawn size of the
  // texture. If we get called due to this resizing don't make the texture
  // dirty. Otherwise, rendering will fail.
  // TODO(tiborg): remove once frame update phases are established.
  float tolerance = 0.00001;
  if (ApproximatelyEqual(width, prev_size.width(), tolerance) &&
      ApproximatelyEqual(height, prev_size.height(), tolerance)) {
    return;
  }
  texture_->SetSize(width, height);
  tooltip_->SetSize(width, tooltip_->size().height());
}

void TabItem::OnHoverEnter(const gfx::PointF& position) {
  SetTranslate(0, 0, 0.05f);
  tooltip_->SetVisible(true);
  tooltip_->SetOpacity(1.0f);
}

void TabItem::OnHoverLeave() {
  SetTranslate(0, 0, 0.001f);
  tooltip_->SetVisible(false);
  tooltip_->SetOpacity(0.0f);
}

void TabItem::SetTitle(const base::string16& title) {
  texture_->SetTitle(title);
  tooltip_->SetText(title);
}

void TabItem::SetColor(SkColor color) {
  texture_->SetColor(color);
}

void TabItem::SetImageId(int image_id) {
  texture_->SetImageId(image_id);
}

UiTexture* TabItem::GetTexture() const {
  return texture_.get();
}

}  // namespace vr
