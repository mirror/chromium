// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/text.h"

#include "base/memory/ptr_util.h"
#include "cc/paint/skia_paint_canvas.h"
#include "chrome/browser/vr/elements/ui_texture.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/vector2d.h"
#include "ui/gfx/render_text.h"

namespace vr {

constexpr float kBorderFactor = 0.045;
constexpr float kFontSizeFactor = 0.048;
constexpr float kTextWidthFactor = 1.0 - 3 * kBorderFactor;

class TextTexture : public UiTexture {
 public:
  explicit TextTexture(int resource_id) : resource_id_(resource_id) {}
  ~TextTexture() override {}

 private:
  gfx::Size GetPreferredTextureSize(int width) const override {
    return gfx::Size(width, width);
  }

  gfx::SizeF GetDrawnSize() const override { return size_; }

  void Draw(SkCanvas* sk_canvas, const gfx::Size& texture_size) override;

  gfx::SizeF size_;
  int resource_id_;

  DISALLOW_COPY_AND_ASSIGN(TextTexture);
};

Text::Text(int maximum_width, int resource_id)
    : TexturedElement(maximum_width),
      texture_(base::MakeUnique<TextTexture>(resource_id)) {}
Text::~Text() {}

UiTexture* Text::GetTexture() const {
  return texture_.get();
}

void TextTexture::Draw(SkCanvas* sk_canvas, const gfx::Size& texture_size) {
  cc::SkiaPaintCanvas paint_canvas(sk_canvas);
  gfx::Canvas gfx_canvas(&paint_canvas, 1.0f);
  gfx::Canvas* canvas = &gfx_canvas;

  size_.set_width(texture_size.width());
  SkPaint paint;
  paint.setColor(SK_ColorTRANSPARENT);
  gfx::FontList fonts;
  base::string16 text = l10n_util::GetStringUTF16(resource_id_);
  GetFontList(size_.width() * kFontSizeFactor, text, &fonts);
  gfx::Rect text_size(size_.width() * kTextWidthFactor, 0);

  std::vector<std::unique_ptr<gfx::RenderText>> lines =
      PrepareDrawStringRect(text, fonts, SK_ColorBLACK, &text_size,
                            kTextAlignmentCenter, kWrappingBehaviorWrap);

  DCHECK_LE(text_size.height(),
            static_cast<int>((1.0 - 2 * kBorderFactor) * size_.width()));
  size_.set_height(size_.width() * 2 * kBorderFactor + text_size.height());
  float radius = size_.height() * kBorderFactor;
  sk_canvas->drawRoundRect(SkRect::MakeWH(size_.width(), size_.height()),
                           radius, radius, paint);

  canvas->Save();
  canvas->Translate(gfx::Vector2d(size_.width() * kBorderFactor,
                                  size_.width() * kBorderFactor));
  for (auto& render_text : lines)
    render_text->Draw(canvas);
  canvas->Restore();
}

}  // namespace vr
