// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/alert_dialog_texture.h"

#include "base/strings/utf_string_conversions.h"
#include "cc/paint/skia_paint_canvas.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/font_list.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/vector2d.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/render_text.h"
#include "ui/gfx/vector_icon_types.h"

namespace vr {

namespace {

constexpr int kHeightWidthRatio = 6.0;
constexpr float kBorderFactor = 0.05;
constexpr float kIconSizeFactor = 0.2;
constexpr float kIconWidthSizeFactor = 0.05;
constexpr float kButtonSizeFactor = 0.2;
constexpr float kFontSizeFactor = 0.10;
constexpr float kTextHeightFactor = 0.3;
constexpr float kTextWidthFactor =
    1.0 - 3 * kBorderFactor - kIconWidthSizeFactor;

}  // namespace

AlertDialogTexture::AlertDialogTexture(const gfx::VectorIcon& icon,
                                       base::string16 text_message,
                                       base::string16 second_text_message,
                                       base::string16 button_message,
                                       base::string16 button2_message)
    : icon_(icon),
      title_text_(text_message),
      toggle_text_(second_text_message),
      b_positive_text_(button_message),
      has_text_(true) {
  b_negative_background_ = color_scheme().system_indicator_background;
  b_positive_background_ = color_scheme().system_indicator_background;
  b_negative_color_ = color_scheme().system_indicator_foreground;
  b_positive_color_ = color_scheme().system_indicator_foreground;
}

AlertDialogTexture::AlertDialogTexture(const gfx::VectorIcon& icon)
    : icon_(icon), has_text_(false) {}

AlertDialogTexture::~AlertDialogTexture() = default;

void AlertDialogTexture::SetContent(long icon,
                                    base::string16 title_text,
                                    base::string16 toggle_text,
                                    int b_positive,
                                    base::string16 b_positive_text,
                                    int b_negative,
                                    base::string16 b_negative_text) {
  title_text_ = title_text;
  toggle_text_ = toggle_text;
  b_positive_text_ = b_positive_text;
  b_negative_text_ = b_negative_text;
  set_dirty();
}

void AlertDialogTexture::Draw(SkCanvas* sk_canvas,
                              const gfx::Size& texture_size) {
  cc::SkiaPaintCanvas paint_canvas(sk_canvas);
  gfx::Canvas gfx_canvas(&paint_canvas, 1.0f);
  gfx::Canvas* canvas = &gfx_canvas;

  size_.set_height(texture_size.height());

  SkPaint paint;
  paint.setColor(color_scheme().system_indicator_background);

  gfx::Rect text_size(0, 0);
  std::vector<std::unique_ptr<gfx::RenderText>> main_line;
  std::vector<std::unique_ptr<gfx::RenderText>> second_line;
  std::vector<std::unique_ptr<gfx::RenderText>> buttons_text;
  std::vector<std::unique_ptr<gfx::RenderText>> buttons2_text;

  gfx::FontList fonts;
  GetFontList(size_.height() * kFontSizeFactor, title_text_, &fonts);
  text_size.set_height(kTextHeightFactor * size_.height());

  main_line = PrepareDrawStringRect(
      title_text_, fonts, color_scheme().system_indicator_foreground,
      &text_size, kTextAlignmentNone, kWrappingBehaviorNoWrap);

  second_line = PrepareDrawStringRect(
      toggle_text_, fonts, color_scheme().system_indicator_foreground,
      &text_size, kTextAlignmentNone, kWrappingBehaviorNoWrap);

  buttons_text = PrepareDrawStringRect(
      b_positive_text_, fonts, b_positive_color_, &text_size,
      kTextAlignmentNone, kWrappingBehaviorNoWrap);

  buttons2_text = PrepareDrawStringRect(
      b_negative_text_, fonts, b_negative_color_, &text_size,
      kTextAlignmentNone, kWrappingBehaviorNoWrap);

  //  DCHECK_LE(text_size.width(), kTextWidthFactor * size_.height());
  LOG(ERROR) << "VRP: " << text_size.width() << ", "
             << kTextWidthFactor * size_.height();

  // Setting background size giving some extra lateral padding to the text.
  //  size_.set_width((3 * kBorderFactor + kIconSizeFactor) *
  //                   size_.height() + text_size.width());
  size_.set_width(text_size.width() * 1 / kTextWidthFactor);

  // Background
  float radius = size_.height() * kBorderFactor;
  sk_canvas->drawRoundRect(SkRect::MakeWH(size_.width(), size_.height()),
                           radius, radius, paint);

  // Icon
  canvas->Save();
  canvas->Translate(gfx::Vector2d(
      IsRTL() ? 4 * kBorderFactor * size_.width() + text_size.width()
              : size_.width() * kBorderFactor,
      size_.height() * kBorderFactor));
  PaintVectorIcon(canvas, icon_, size_.height() * kIconSizeFactor,
                  color_scheme().system_indicator_foreground);
  canvas->Restore();

  // main msg lines
  canvas->Save();
  canvas->Translate(gfx::Vector2d(
      size_.width() * (IsRTL() ? 2 * kBorderFactor
                               : 3 * kBorderFactor + kIconWidthSizeFactor),
      size_.height() * kBorderFactor));
  for (auto& render_text : main_line)
    render_text->Draw(canvas);

  canvas->Restore();

  // second msg lines
  canvas->Save();
  canvas->Translate(gfx::Vector2d(
      size_.width() * (IsRTL() ? 2 * kBorderFactor
                               : 3 * kBorderFactor + kIconWidthSizeFactor),
      size_.height() * (2 * kBorderFactor + kIconSizeFactor)));
  for (auto& render_text : second_line)
    render_text->Draw(canvas);

  canvas->Restore();

  // buttons
  canvas->Save();
  canvas->Translate(gfx::Vector2d(
      size_.width() * (IsRTL() ? 2 * kBorderFactor
                               : (1 - 1 * kBorderFactor - kButtonSizeFactor)),
      size_.height() * (4 * kBorderFactor + 2 * kIconSizeFactor)));
  for (auto& render_text : buttons_text)
    render_text->Draw(canvas);
  canvas->Restore();

  // buttons
  canvas->Save();
  canvas->Translate(gfx::Vector2d(
      size_.width() * (IsRTL()
                           ? 2 * kBorderFactor
                           : (1 - 2 * kBorderFactor - 2 * kButtonSizeFactor)),
      size_.height() * (4 * kBorderFactor + 2 * kIconSizeFactor)));
  for (auto& render_text : buttons2_text)
    render_text->Draw(canvas);
  canvas->Restore();
}

gfx::Size AlertDialogTexture::GetPreferredTextureSize(int maximum_width) const {
  // All indicators need to be the same height, so compute height, and then
  // re-compute with based on whether the indicator has text or not.
  int height = maximum_width / kHeightWidthRatio;
  return gfx::Size(has_text_ ? maximum_width : height, height);
}

gfx::SizeF AlertDialogTexture::GetDrawnSize() const {
  return size_;
}

void AlertDialogTexture::UpdateHoverState(const gfx::PointF& position) {
  if (position.x() > (1 - 1 * kBorderFactor - kButtonSizeFactor) &&
      position.x() < (1 - 1 * kBorderFactor) &&
      position.y() > (4 * kBorderFactor + 2 * kIconSizeFactor) &&
      position.y() <
          (4 * kBorderFactor + 2 * kIconSizeFactor + kButtonSizeFactor)) {
    b_positive_color_ = color_scheme().alert_button_background;
  } else
    b_positive_color_ = color_scheme().system_indicator_foreground;

  if (position.x() > (1 - 1 * kBorderFactor - kButtonSizeFactor) &&
      position.x() < (1 - 1 * kBorderFactor) &&
      position.y() > (4 * kBorderFactor + 2 * kIconSizeFactor) &&
      position.y() <
          (4 * kBorderFactor + 2 * kIconSizeFactor + kButtonSizeFactor)) {
    b_positive_color_ = color_scheme().alert_button_background;
  } else
    b_positive_color_ = color_scheme().system_indicator_foreground;

  set_dirty();
}

}  // namespace vr
