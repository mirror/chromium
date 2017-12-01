// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/text_input.h"

#include "base/memory/ptr_util.h"
#include "chrome/browser/vr/elements/rect.h"
#include "chrome/browser/vr/elements/text.h"
#include "chrome/browser/vr/elements/ui_texture.h"
#include "ui/gfx/geometry/rect.h"

namespace vr {

namespace {

static constexpr int kCursorBlinkHalfPeriodMs = 600;
}

TextInput::TextInput(int texture_width_pixels, float font_height_meters) {
  auto text = base::MakeUnique<Text>(texture_width_pixels, font_height_meters);
  text->set_type(kTypeTextInputHint);
  text->set_draw_phase(kPhaseForeground);
  text->set_hit_testable(false);
  text->set_x_anchoring(LEFT);
  text->set_x_centering(LEFT);
  text->SetSize(1, 1);
  text->SetMultiLine(false);
  text->SetTextAlignment(UiTexture::kTextAlignmentLeft);
  hint_element_ = text.get();
  this->AddChild(std::move(text));

  text = base::MakeUnique<Text>(texture_width_pixels, font_height_meters);
  text->set_type(kTypeTextInputText);
  text->set_draw_phase(kPhaseForeground);
  text->set_hit_testable(false);
  text->set_x_anchoring(LEFT);
  text->set_x_centering(LEFT);
  text->SetSize(1, 1);
  text->SetMultiLine(false);
  text->SetTextAlignment(UiTexture::kTextAlignmentLeft);
  text->SetCursorEnabled(true);
  text_element_ = text.get();
  this->AddChild(std::move(text));

  auto cursor = base::MakeUnique<Rect>();
  cursor->set_type(kTypeTextInputCursor);
  cursor->set_draw_phase(kPhaseForeground);
  cursor->set_hit_testable(false);
  cursor->set_x_anchoring(LEFT);
  cursor->set_y_anchoring(BOTTOM);
  cursor->SetColor(SK_ColorBLUE);
  cursor_element_ = cursor.get();
  text_element_->AddChild(std::move(cursor));

  set_bounds_contain_children(true);
}

TextInput::~TextInput() {}

void TextInput::SetText(const base::string16& text) {
  if (text_ == text)
    return;
  text_ = text;

  text_element_->SetText(text);
  hint_element_->SetVisible(text_.empty());

  if (text_changed_callback_)
    text_changed_callback_.Run(text);
}

void TextInput::SetHintText(const base::string16& text) {
  hint_element_->SetText(text);
}

void TextInput::SetCursorPosition(int position) {
  text_element_->SetCursorPosition(position);
}

void TextInput::SetTextChangedCallback(const TextInputCallback& callback) {
  text_changed_callback_ = callback;
}

void TextInput::SetTextColor(SkColor color) {
  text_element_->SetColor(color);
}

void TextInput::SetCursorColor(SkColor color) {
  cursor_element_->SetColor(color);
}

bool TextInput::OnBeginFrame(const base::TimeTicks& time,
                             const gfx::Vector3dF& look_at) {
  return SetCursorBlinkState(time);
}

void TextInput::OnSetSize(gfx::SizeF size) {
  hint_element_->SetSize(size.width(), size.height());
  text_element_->SetSize(size.width(), size.height());
}

void TextInput::OnSetName() {
  hint_element_->set_owner_name_for_test(name());
  text_element_->set_owner_name_for_test(name());
  cursor_element_->set_owner_name_for_test(name());
}

void TextInput::LayOutChildren() {
  // To avoid re-rendering a texture when the cursor blinks, the texture is a
  // separate element. Once the text has been laid out, we can position the
  // cursor appropriately relative to the text field.
  gfx::RectF bounds = text_element_->GetCursorBounds();
  cursor_element_->SetTranslate(bounds.x(), bounds.y(), 0);
  cursor_element_->SetSize(bounds.width(), bounds.height());
}

bool TextInput::SetCursorBlinkState(const base::TimeTicks& time) {
  base::TimeDelta delta = time - base::TimeTicks();
  bool visible = delta.InMilliseconds() / kCursorBlinkHalfPeriodMs % 2;
  if (cursor_visible_ == visible)
    return false;
  cursor_visible_ = visible;
  cursor_element_->SetVisible(visible);
  return true;
}

}  // namespace vr
