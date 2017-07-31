// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/tab_item_texture.h"

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/codec/jpeg_codec.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/render_text.h"

namespace vr {

TabItemTexture::TabItemTexture() = default;

TabItemTexture::~TabItemTexture() = default;

void TabItemTexture::Draw(SkCanvas* sk_canvas, const gfx::Size& texture_size) {
  if (size_.height() < size_.width()) {
    pixel_size_.set_width(texture_size.width());
    pixel_size_.set_height(texture_size.height() * size_.height() /
                           size_.width());
  } else {
    pixel_size_.set_width(texture_size.width() * size_.width() /
                          size_.height());
    pixel_size_.set_height(texture_size.height());
  }

  // Clip all drawing to rounded rect.
  float radius = 20.0f;
  sk_canvas->clipRRect(SkRRect::MakeRectXY(
      SkRect::MakeXYWH(0, 0, pixel_size_.width(), pixel_size_.height()), radius,
      radius));

  // Load and draw image if we have an image ID. Otherwise draw solid color
  // only.
  SkColor color = color_;
  if (image_id_ != -1) {
    base::StringPiece data =
        ResourceBundle::GetSharedInstance().GetRawDataResource(image_id_);

    std::unique_ptr<SkBitmap> bitmap = gfx::JPEGCodec::Decode(
        reinterpret_cast<const unsigned char*>(data.data()), data.size());
    DCHECK(bitmap);

    auto image = SkImage::MakeFromBitmap(*bitmap);
    sk_canvas->drawImage(image, 0.0f, 0.0f);

    color = SkColorSetA(color, SkColorGetA(color) / 2);
  }

  SkPaint paint;
  paint.setColor(color);
  sk_canvas->drawRoundRect(
      SkRect::MakeXYWH(0, 0, pixel_size_.width(), pixel_size_.height()), radius,
      radius, paint);

  // Draw first title letter.
  cc::SkiaPaintCanvas paint_canvas(sk_canvas);
  gfx::Canvas gfx_canvas(&paint_canvas, 1.0f);
  gfx::Canvas* canvas = &gfx_canvas;
  auto text = base::ToUpperASCII((title_.size() > 0) ? title_.substr(0, 1)
                                                     : base::string16());
  gfx::FontList fonts;
  constexpr int kFontSize = 300;
  GetFontList(kFontSize, text, &fonts);
  gfx::Rect prompt_text_size(pixel_size_.width(), 0);
  std::vector<std::unique_ptr<gfx::RenderText>> lines =
      PrepareDrawStringRect(text, fonts, 0xB0e8e8e8, &prompt_text_size,
                            kTextAlignmentCenter, kWrappingBehaviorWrap);
  canvas->Save();
  canvas->Translate(gfx::Vector2d(
      0, pixel_size_.height() / 2 - kFontSize / 2 - pixel_size_.height() / 15));
  for (auto& render_text : lines) {
    render_text->Draw(canvas);
  }
  canvas->Restore();
}

gfx::Size TabItemTexture::GetPreferredTextureSize(int maximum_width) const {
  return gfx::Size(maximum_width, maximum_width);
}

gfx::SizeF TabItemTexture::GetDrawnSize() const {
  return pixel_size_;
}

void TabItemTexture::SetSize(float width, float height) {
  size_.set_width(width);
  size_.set_height(height);
  set_dirty();
}

void TabItemTexture::SetTitle(const base::string16& title) {
  title_ = title;
  set_dirty();
}

void TabItemTexture::SetColor(SkColor color) {
  color_ = color;
  set_dirty();
}

void TabItemTexture::SetImageId(int image_id) {
  image_id_ = image_id;
  set_dirty();
}

}  // namespace vr
