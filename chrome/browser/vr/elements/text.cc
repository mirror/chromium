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

namespace {
constexpr float kCursorWidthRatio = 0.07f;
constexpr int kTextPixelPerDMM = 1100;
}

namespace vr {

class TextTexture : public UiTexture {
 public:
  explicit TextTexture(float font_height_dmms)
      : font_height_dmms_(font_height_dmms) {}
  ~TextTexture() override {}

  void SetText(const base::string16& text) { SetAndDirty(&text_, text); }

  void SetColor(SkColor color) { SetAndDirty(&color_, color); }

  void SetAlignment(TextAlignment alignment) {
    SetAndDirty(&alignment_, alignment);
  }

  void SetTextMode(TextMode mode) { SetAndDirty(&mode_, mode); }

  void SetCursorEnabled(bool enabled) {
    SetAndDirty(&cursor_enabled_, enabled);
  }

  void SetCursorPosition(int position) {
    SetAndDirty(&cursor_position_, position);
  }

  void SetTextSize(const gfx::SizeF& size) { text_size_ = size; }

  gfx::SizeF GetDrawnSize() const override { return size_; }

  gfx::Rect get_cursor_bounds() { return cursor_bounds_; }

  // This method does all text preparation for the element other than drawing to
  // the texture. This allows for deeper unit testing of the Text element
  // without having to mock canvases and simulate frame rendering. The state of
  // the texture is modified here.
  void LayOutText();

  const std::vector<std::unique_ptr<gfx::RenderText>>& lines() {
    return lines_;
  }

 private:
  gfx::Size GetPreferredTextureSize(int width) override {
    LayOutText();
    return gfx::Size(GetDrawnSize().width(), GetDrawnSize().height());
  }

  void Draw(SkCanvas* sk_canvas, const gfx::Size& texture_size) override;

  gfx::SizeF size_;
  base::string16 text_;
  // These dimensions are in meters.
  float font_height_dmms_ = 0;
  TextAlignment alignment_ = kTextAlignmentCenter;
  TextMode mode_ = kMultiLineFixedWidth;
  gfx::SizeF text_size_;
  SkColor color_ = SK_ColorBLACK;
  bool cursor_enabled_ = false;
  int cursor_position_ = 0;
  gfx::Rect cursor_bounds_;
  std::vector<std::unique_ptr<gfx::RenderText>> lines_;

  DISALLOW_COPY_AND_ASSIGN(TextTexture);
};

Text::Text(float font_height_dmms)
    : TexturedElement(0),
      texture_(base::MakeUnique<TextTexture>(font_height_dmms)) {}
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

void Text::SetTextMode(TextMode mode) {
  text_mode_ = mode;
  texture_->SetTextMode(mode);
}

void Text::SetCursorEnabled(bool enabled) {
  texture_->SetCursorEnabled(enabled);
}

void Text::SetCursorPosition(int position) {
  texture_->SetCursorPosition(position);
}

gfx::Rect Text::GetRawCursorBounds() const {
  return texture_->get_cursor_bounds();
}

gfx::RectF Text::GetCursorBounds() const {
  // Note that gfx:: cursor bounds always indicate a one-pixel width, so we
  // override the width here to be a percentage of height for the sake of
  // arbitrary texture sizes.
  gfx::Rect bounds = texture_->get_cursor_bounds();
  float scale = size().width() / texture_->GetDrawnSize().width();
  return gfx::RectF(
      bounds.CenterPoint().x() * scale, bounds.CenterPoint().y() * scale,
      bounds.height() * scale * kCursorWidthRatio, bounds.height() * scale);
}

void Text::OnSetSize(gfx::SizeF size) {
  texture_->SetTextSize(size);
}

void Text::UpdateElementSize() {
  gfx::SizeF drawn_size = GetTexture()->GetDrawnSize();
  SetSize(drawn_size.width() / kTextPixelPerDMM,
          drawn_size.height() / kTextPixelPerDMM);
}

const std::vector<std::unique_ptr<gfx::RenderText>>& Text::LayOutTextForTest() {
  texture_->LayOutText();
  return texture_->lines();
}

gfx::SizeF Text::GetTextureSizeForTest() const {
  return texture_->GetDrawnSize();
}

UiTexture* Text::GetTexture() const {
  return texture_.get();
}

void TextTexture::LayOutText() {
  gfx::FontList fonts;
  int pixel_font_height =
      static_cast<int>(font_height_dmms_ * kTextPixelPerDMM);
  GetDefaultFontList(pixel_font_height, text_, &fonts);
  gfx::Rect text_bounds;
  if (mode_ == kSingleLineFixedHeight) {
    text_bounds.set_height(pixel_font_height);
  } else {
    text_bounds.set_width(kTextPixelPerDMM * text_size_.width());
  }

  TextRenderParameters parameters;
  parameters.color = color_;
  parameters.text_alignment = alignment_;
  parameters.wrapping_behavior = mode_ == kMultiLineFixedWidth
                                     ? kWrappingBehaviorWrap
                                     : kWrappingBehaviorNoWrap;
  parameters.cursor_enabled = cursor_enabled_;
  parameters.cursor_position = cursor_position_;

  lines_ =
      // TODO(vollick): if this subsumes all text, then we should probably move
      // this function into this class.
      PrepareDrawStringRect(text_, fonts, &text_bounds, parameters);

  if (cursor_enabled_)
    cursor_bounds_ = lines_.front()->GetUpdatedCursorBounds();

  // Note, there is no padding here whatsoever.
  size_ = gfx::SizeF(text_bounds.size());
}

void TextTexture::Draw(SkCanvas* sk_canvas, const gfx::Size& texture_size) {
  cc::SkiaPaintCanvas paint_canvas(sk_canvas);
  gfx::Canvas gfx_canvas(&paint_canvas, 1.0f);
  gfx::Canvas* canvas = &gfx_canvas;

  for (auto& render_text : lines_)
    render_text->Draw(canvas);
}

}  // namespace vr
