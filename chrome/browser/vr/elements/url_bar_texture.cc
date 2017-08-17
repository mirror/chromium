// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/url_bar_texture.h"

#include "cc/paint/skia_paint_canvas.h"
#include "chrome/browser/vr/color_scheme.h"
#include "chrome/browser/vr/vr_url_util.h"
#include "components/vector_icons/vector_icons.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/point_f.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/render_text.h"

namespace vr {

namespace {

static constexpr float kWidth = 0.672;
static constexpr float kHeight = 0.088;
static constexpr float kFontHeight = 0.027;
static constexpr float kBackButtonWidth = kHeight;
static constexpr float kBackIconHeight = 0.0375;
static constexpr float kBackIconOffset = 0.005;
static constexpr float kFieldSpacing = 0.014;
static constexpr float kSecurityIconSize = 0.03;
static constexpr float kUrlRightMargin = 0.02;
static constexpr float kSeparatorWidth = 0.002;
static constexpr float kChipTextLineMargin = kHeight * 0.3;

gfx::PointF percentToMeters(const gfx::PointF& percent) {
  return gfx::PointF(percent.x() * kWidth, percent.y() * kHeight);
}

}  // namespace

UrlBarTexture::UrlBarTexture(
    const base::Callback<void(UiUnsupportedMode)>& failure_callback)
    : failure_callback_(failure_callback) {}

UrlBarTexture::~UrlBarTexture() = default;

void UrlBarTexture::SetToolbarState(const ToolbarState& state) {
  if (state_ == state)
    return;
  state_ = state;
  url_dirty_ = true;
  set_dirty();
}

void UrlBarTexture::SetHistoryButtonsEnabled(bool can_go_back) {
  if (can_go_back != can_go_back_)
    set_dirty();
  can_go_back_ = can_go_back;
}

float UrlBarTexture::ToPixels(float meters) const {
  return meters * size_.width() / kWidth;
}

float UrlBarTexture::ToMeters(float pixels) const {
  return pixels * kWidth / size_.width();
}

bool UrlBarTexture::HitsBackButton(const gfx::PointF& position) const {
  const gfx::PointF& meters = percentToMeters(position);
  return back_button_hit_region_.Contains(meters) &&
         !HitsTransparentRegion(meters, true);
}

bool UrlBarTexture::HitsUrlBar(const gfx::PointF& position) const {
  const gfx::PointF& meters = percentToMeters(position);
  gfx::RectF rect(gfx::PointF(kBackButtonWidth, 0),
                  gfx::SizeF(kWidth - kBackButtonWidth, kHeight));
  return rect.Contains(meters) && !HitsTransparentRegion(meters, false);
}

bool UrlBarTexture::HitsSecurityRegion(const gfx::PointF& position) const {
  return security_hit_region_.Contains(percentToMeters(position));
}

bool UrlBarTexture::HitsTransparentRegion(const gfx::PointF& meters,
                                          bool left) const {
  const float radius = kHeight / 2.0f;
  gfx::PointF circle_center(left ? radius : kWidth - radius, radius);
  if (!left && meters.x() < circle_center.x())
    return false;
  if (left && meters.x() > circle_center.x())
    return false;
  return (meters - circle_center).LengthSquared() > radius * radius;
}

void UrlBarTexture::SetBackButtonHovered(bool hovered) {
  if (back_hovered_ != hovered)
    set_dirty();
  back_hovered_ = hovered;
}

void UrlBarTexture::SetBackButtonPressed(bool pressed) {
  if (back_pressed_ != pressed)
    set_dirty();
  back_pressed_ = pressed;
}

SkColor UrlBarTexture::GetLeftCornerColor() const {
  SkColor color = color_scheme().element_background;
  if (can_go_back_) {
    if (back_pressed_)
      color = color_scheme().element_background_down;
    else if (back_hovered_)
      color = color_scheme().element_background_hover;
  }
  return color;
}

void UrlBarTexture::OnSetMode() {
  url_dirty_ = true;
  set_dirty();
}

void UrlBarTexture::Draw(SkCanvas* canvas, const gfx::Size& texture_size) {
  size_.set_height(texture_size.height());
  size_.set_width(texture_size.width());

  rendered_url_text_ = base::string16();
  rendered_url_text_rect_ = gfx::Rect();
  rendered_security_text_ = base::string16();
  rendered_security_text_rect_ = gfx::Rect();

  canvas->save();
  canvas->scale(size_.width() / kWidth, size_.width() / kWidth);

  // Make a gfx canvas to support utility drawing methods.
  cc::SkiaPaintCanvas paint_canvas(canvas);
  gfx::Canvas gfx_canvas(&paint_canvas, 1.0f);

  // Left rounded corner of URL bar.
  SkRRect round_rect;
  SkVector rounded_corner = {kHeight / 2, kHeight / 2};
  SkVector left_corners[4] = {rounded_corner, {0, 0}, {0, 0}, rounded_corner};
  round_rect.setRectRadii({0, 0, kHeight, kHeight}, left_corners);
  SkPaint paint;
  paint.setColor(GetLeftCornerColor());
  canvas->drawRRect(round_rect, paint);

  // URL area.
  paint.setColor(color_scheme().element_background);

  SkVector right_corners[4] = {{0, 0}, rounded_corner, rounded_corner, {0, 0}};
  round_rect.setRectRadii({kHeight, 0, kWidth, kHeight}, right_corners);
  canvas->drawRRect(round_rect, paint);

  back_button_hit_region_.SetRect(0, 0, 0, 0);
  security_hit_region_.SetRect(0, 0, 0, 0);

  // Keep track of a left edge as we selectively render components of the URL
  // bar left-to-right.
  float left_edge = 0;

  // Back button / URL separator vertical line.
  paint.setColor(color_scheme().separator);
  canvas->drawRect(
      SkRect::MakeXYWH(kBackButtonWidth, 0, kSeparatorWidth, kHeight), paint);

  // Back button icon.
  canvas->save();
  canvas->translate(kBackButtonWidth / 2 + kBackIconOffset, kHeight / 2);
  canvas->translate(-kBackIconHeight / 2, -kBackIconHeight / 2);
  float icon_scale = kBackIconHeight /
                     GetDefaultSizeOfVectorIcon(vector_icons::kBackArrowIcon);
  canvas->scale(icon_scale, icon_scale);
  PaintVectorIcon(&gfx_canvas, vector_icons::kBackArrowIcon,
                  can_go_back_ ? color_scheme().element_foreground
                               : color_scheme().disabled);
  canvas->restore();

  back_button_hit_region_.SetRect(left_edge, 0, left_edge + kBackButtonWidth,
                                  kHeight);
  left_edge += kBackButtonWidth + kSeparatorWidth;

  // Site security state icon.
  left_edge += kFieldSpacing;
  if ((state_.security_level != security_state::NONE || state_.offline_page) &&
      state_.vector_icon != nullptr && state_.should_display_url) {
    gfx::RectF icon_region(left_edge, kHeight / 2 - kSecurityIconSize / 2,
                           kSecurityIconSize, kSecurityIconSize);
    canvas->save();
    canvas->translate(icon_region.x(), icon_region.y());
    const gfx::VectorIcon& icon = *state_.vector_icon;
    float icon_scale = kSecurityIconSize / GetDefaultSizeOfVectorIcon(icon);
    canvas->scale(icon_scale, icon_scale);
    PaintVectorIcon(
        &gfx_canvas, icon,
        vr::GetUrlSecurityChipColor(state_.security_level, state_.offline_page,
                                    color_scheme()));
    canvas->restore();

    security_hit_region_ = icon_region;
    left_edge += kSecurityIconSize + kFieldSpacing;
  }

  canvas->restore();

  // The security chip text consumes a significant percentage of URL bar text
  // space, so it is currently disabled (see crbug.com/734206). The offline
  // state is an exception, and must be shown (see crbug.com/735770).
  bool draw_security_chip = state_.offline_page;

  // Possibly draw security chip text (eg. "Not secure") next to the security
  // icon.
  if (draw_security_chip && state_.should_display_url) {
    float chip_max_width = kWidth - left_edge - kUrlRightMargin;
    gfx::Rect text_bounds(ToPixels(left_edge), 0, ToPixels(chip_max_width),
                          ToPixels(kHeight));

    int pixel_font_height = texture_size.height() * kFontHeight / kHeight;
    SkColor chip_color = GetUrlSecurityChipColor(
        state_.security_level, state_.offline_page, color_scheme());
    const base::string16& chip_text = state_.secure_verbose_text;
    DCHECK(!chip_text.empty());

    gfx::FontList font_list;
    if (!GetFontList(pixel_font_height, chip_text, &font_list))
      failure_callback_.Run(UiUnsupportedMode::kUnhandledCodePoint);

    std::unique_ptr<gfx::RenderText> render_text(
        gfx::RenderText::CreateInstance());
    render_text->SetFontList(font_list);
    render_text->SetColor(chip_color);
    render_text->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    render_text->SetText(chip_text);
    render_text->SetDisplayRect(text_bounds);
    render_text->Draw(&gfx_canvas);
    rendered_security_text_ = render_text->text();
    rendered_security_text_rect_ = render_text->display_rect();

    // Capture the rendered text region for future hit testing.
    gfx::Size string_size = render_text->GetStringSize();
    gfx::RectF hit_bounds(
        left_edge, kHeight / 2 - ToMeters(string_size.height()) / 2,
        ToMeters(string_size.width()), ToMeters(string_size.height()));
    security_hit_region_.Union(hit_bounds);
    left_edge += ToMeters(string_size.width());

    // Separator line between security text and URL.
    left_edge += kFieldSpacing;
    paint.setColor(color_scheme().url_deemphasized);
    canvas->drawRect(
        SkRect::MakeXYWH(ToPixels(left_edge), ToPixels(kChipTextLineMargin),
                         ToPixels(kSeparatorWidth),
                         ToPixels(kHeight - 2 * kChipTextLineMargin)),
        paint);
    left_edge += kFieldSpacing + kSeparatorWidth;
  }

  if (state_.should_display_url) {
    float url_x = left_edge;
    if (!url_render_text_ || url_dirty_) {
      float url_width = kWidth - url_x - kUrlRightMargin;
      gfx::Rect text_bounds(ToPixels(url_x), 0, ToPixels(url_width),
                            ToPixels(kHeight));
      url_formatter::FormatUrlTypes format_types =
          url_formatter::kFormatUrlOmitDefaults;
      if (state_.offline_page)
        format_types |= url_formatter::kFormatUrlOmitHTTPS;
      std::unique_ptr<gfx::RenderText> render_text(
          gfx::RenderText::CreateInstance());
      render_text->SetDisplayRect(text_bounds);
      int pixel_font_height = texture_size.height() * kFontHeight / kHeight;
      vr::RenderUrl(state_.gurl, format_types, state_.security_level,
                    color_scheme(), failure_callback_, pixel_font_height,
                    SK_ColorBLACK, true, render_text.get());
      url_render_text_ = std::move(render_text);
      url_dirty_ = false;
    }
    url_render_text_->Draw(&gfx_canvas);
    rendered_url_text_ = url_render_text_->text();
    rendered_url_text_rect_ = url_render_text_->display_rect();
  }
}

gfx::Size UrlBarTexture::GetPreferredTextureSize(int maximum_width) const {
  return gfx::Size(maximum_width, maximum_width * kHeight / kWidth);
}

gfx::SizeF UrlBarTexture::GetDrawnSize() const {
  return size_;
}

}  // namespace vr
