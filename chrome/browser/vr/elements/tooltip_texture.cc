// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/tooltip_texture.h"

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/codec/jpeg_codec.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/render_text.h"

namespace vr {

TooltipTexture::TooltipTexture() = default;

TooltipTexture::~TooltipTexture() = default;

void TooltipTexture::Draw(SkCanvas* sk_canvas, const gfx::Size& texture_size) {
  if (size_.height() < size_.width()) {
    pixel_size_.set_width(texture_size.width());
    pixel_size_.set_height(texture_size.height() * size_.height() /
                           size_.width());
  } else {
    pixel_size_.set_width(texture_size.width() * size_.width() /
                          size_.height());
    pixel_size_.set_height(texture_size.height());
  }

  // Draw background.
  float radius = 10.0f;
  SkPaint paint;
  paint.setColor(background_color_);
  sk_canvas->drawRoundRect(
      SkRect::MakeXYWH(0, 0, pixel_size_.width(), pixel_size_.height()), radius,
      radius, paint);

  // Draw text.
  cc::SkiaPaintCanvas paint_canvas(sk_canvas);
  gfx::Canvas gfx_canvas(&paint_canvas, 1.0f);
  gfx::Canvas* canvas = &gfx_canvas;
  gfx::FontList fonts;
  constexpr int kFontSize = 50;
  constexpr float kPadding = 10;
  GetFontList(kFontSize, text_, &fonts);
  gfx::Rect bounds(pixel_size_.width() - kPadding * 2,
                   pixel_size_.height() - kPadding * 2);

  std::unique_ptr<gfx::RenderText> render_text(
      gfx::RenderText::CreateInstance());
  render_text->SetFontList(fonts);
  render_text->SetColor(text_color_);
  render_text->SetHorizontalAlignment(gfx::ALIGN_CENTER);
  render_text->SetElideBehavior(gfx::ELIDE_TAIL);
  render_text->SetText(text_);
  render_text->SetDisplayRect(bounds);

  canvas->Save();
  canvas->Translate(gfx::Vector2d(kPadding, kPadding));
  render_text->Draw(canvas);
  canvas->Restore();
}

gfx::Size TooltipTexture::GetPreferredTextureSize(int maximum_width) const {
  return gfx::Size(maximum_width, maximum_width);
}

gfx::SizeF TooltipTexture::GetDrawnSize() const {
  return pixel_size_;
}

void TooltipTexture::SetSize(float width, float height) {
  size_.set_width(width);
  size_.set_height(height);
  set_dirty();
}

void TooltipTexture::SetText(const base::string16& text) {
  text_ = text;
  set_dirty();
}

void TooltipTexture::SetBackgroundColor(SkColor color) {
  background_color_ = color;
  set_dirty();
}

void TooltipTexture::SetTextColor(SkColor color) {
  text_color_ = color;
  set_dirty();
}

}  // namespace vr
