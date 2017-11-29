// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/text.h"

#include "base/memory/ptr_util.h"
#include "cc/paint/skia_paint_canvas.h"
#include "chrome/browser/vr/elements/ui_texture.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/vector2d.h"
#include "ui/gfx/render_text.h"

namespace vr {

namespace {

class TextTexture : public UiTexture {
 public:
  TextTexture(float font_height) : font_height_(font_height) {}
  ~TextTexture() override {}

  void SetText(const base::string16& text) { SetAndDirty(&text_, text); }

  void SetColor(SkColor color) { SetAndDirty(&color_, color); }

  void SetAlignment(TextAlignment alignment) {
    SetAndDirty(&alignment_, alignment);
  }

  void SetMultiLine(bool multiline) {
    if (multiline_ == multiline)
      return;
    multiline_ = multiline;
    set_dirty();
  }

  void SetTextWidthDmm(float width) { text_width_ = width; }

 private:
  gfx::Size GetPreferredTextureSize(int width) const override {
    return gfx::Size(width, width);
  }

  gfx::SizeF GetDrawnSize() const override { return size_; }

  void Draw(SkCanvas* sk_canvas, const gfx::Size& texture_size) override;

  gfx::SizeF size_;
  base::string16 text_;
  // These widths are in meters.
  float font_height_;
  float text_width_;
  TextAlignment alignment_ = kTextAlignmentCenter;
  bool multiline_ = true;

  SkColor color_ = SK_ColorBLACK;

  DISALLOW_COPY_AND_ASSIGN(TextTexture);
};

}  // namespace

Text::Text(int maximum_width_pixels, float font_height_meters)
    : TexturedElement(maximum_width_pixels),
      texture_(base::MakeUnique<TextTexture>(font_height_meters)) {}
Text::~Text() {}

void Text::SetText(const base::string16& text) {
  texture_->SetText(text);
}

void Text::SetColor(SkColor color) {
  texture_->SetColor(color);
}

void Text::SetTextAlignment(UiTexture::TextAlignment alignment) {
  texture_->SetAlignment(alignment);
}

void Text::SetMultiLine(bool multiline) {
  texture_->SetMultiLine(multiline);
}

void Text::OnSetSize(gfx::SizeF size) {
  texture_->SetTextWidthDmm(size.width());
}

UiTexture* Text::GetTexture() const {
  return texture_.get();
}

void TextTexture::Draw(SkCanvas* sk_canvas, const gfx::Size& texture_size) {
  LOG(INFO) << "Drawing text '" << text_ << "':";
  LOG(INFO) << "  Texture size: " << texture_size.ToString();

  cc::SkiaPaintCanvas paint_canvas(sk_canvas);
  gfx::Canvas gfx_canvas(&paint_canvas, 1.0f);
  gfx::Canvas* canvas = &gfx_canvas;

  gfx::FontList fonts;
  float pixels_per_meter = texture_size.width() / text_width_;
  int pixel_font_height = static_cast<int>(font_height_ * pixels_per_meter);
  GetFontList(pixel_font_height, text_, &fonts);

  gfx::Rect text_bounds(texture_size.width(),
                        multiline_ ? 0 : texture_size.height());
  LOG(INFO) << "  Input text bounds: " << text_bounds.size().ToString();

  std::vector<std::unique_ptr<gfx::RenderText>> lines =
      // TODO(vollick): if this subsumes all text, then we should probably move
      // this function into this class.
      PrepareDrawStringRect(
          text_, fonts, color_, &text_bounds, alignment_,
          multiline_ ? kWrappingBehaviorWrap : kWrappingBehaviorNoWrap);

  LOG(INFO) << "  Rendered text bounds: " << text_bounds.size().ToString();

  // Draw the text.
  for (auto& render_text : lines)
    render_text->Draw(canvas);

  // Note, there is no padding here whatsoever.
  size_ = gfx::SizeF(text_bounds.size());
  LOG(INFO) << "  New saved texture size: " << size_.ToString();
}

}  // namespace vr
