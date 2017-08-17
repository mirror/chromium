// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/webvr_url_toast_texture.h"

#include "cc/paint/skia_paint_canvas.h"
#include "chrome/browser/vr/color_scheme.h"
#include "chrome/browser/vr/vr_url_util.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/render_text.h"

namespace vr {

namespace {

static constexpr float kWidth = 0.572;
static constexpr float kHeight = 0.088;
static constexpr float kFontHeight = 0.027;
static constexpr float kFieldSpacing = 0.014;
static constexpr float kSecurityIconSize = 0.03;
static constexpr float kUrlRightMargin = 0.02;
static constexpr float kRadiusFactor = 0.055;

}  // namespace

WebVrUrlToastTexture::WebVrUrlToastTexture(
    const base::Callback<void(UiUnsupportedMode)>& failure_callback)
    : failure_callback_(failure_callback) {}

WebVrUrlToastTexture::~WebVrUrlToastTexture() = default;

void WebVrUrlToastTexture::SetToolbarState(const ToolbarState& state) {
  if (state_ == state)
    return;
  state_ = state;
  url_dirty_ = true;
  set_dirty();
}

float WebVrUrlToastTexture::ToPixels(float meters) const {
  return meters * size_.width() / kWidth;
}

void WebVrUrlToastTexture::Draw(SkCanvas* canvas,
                                const gfx::Size& texture_size) {
  size_.set_height(texture_size.height());
  size_.set_width(texture_size.width());

  // Draw background.
  float radius = size_.height() * kRadiusFactor;
  SkPaint paint;
  paint.setColor(color_scheme().transient_warning_background);
  paint.setAlpha(255);
  canvas->drawRoundRect(
      SkRect::MakeXYWH(0, 0, ToPixels(kWidth), ToPixels(kHeight)), radius,
      radius, paint);

  canvas->save();
  canvas->scale(size_.width() / kWidth, size_.width() / kWidth);

  // Make a gfx canvas to support ldrawing methods.
  cc::SkiaPaintCanvas paint_canvas(canvas);
  gfx::Canvas gfx_canvas(&paint_canvas, 1.0f);

  // Keep track of a left edge as we selectively render components of the URL
  // bar left-to-right.
  float left_edge = 0;

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
    PaintVectorIcon(&gfx_canvas, icon,
                    color_scheme().transient_warning_foreground);
    canvas->restore();
    left_edge += kSecurityIconSize + kFieldSpacing;
  }

  canvas->restore();

  if (state_.should_display_url) {
    float url_x = left_edge;
    if (!url_render_text_ || url_dirty_) {
      float url_width = kWidth - url_x - kUrlRightMargin;
      gfx::Rect text_bounds(ToPixels(url_x), 0, ToPixels(url_width),
                            ToPixels(kHeight));
      url_formatter::FormatUrlTypes format_types =
          url_formatter::kFormatUrlExperimentalElideAfterHost;
      if (state_.offline_page)
        format_types |= url_formatter::kFormatUrlOmitHTTPS;
      std::unique_ptr<gfx::RenderText> render_text(
          gfx::RenderText::CreateInstance());
      render_text->SetDisplayRect(text_bounds);
      int pixel_font_height = texture_size.height() * kFontHeight / kHeight;
      vr::RenderUrl(state_.gurl, format_types, state_.security_level,
                    color_scheme(), failure_callback_, pixel_font_height, false,
                    color_scheme().transient_warning_foreground,
                    render_text.get());
      url_render_text_ = std::move(render_text);
      url_dirty_ = false;
    }
    url_render_text_->Draw(&gfx_canvas);
  }
}

gfx::Size WebVrUrlToastTexture::GetPreferredTextureSize(
    int maximum_width) const {
  return gfx::Size(maximum_width, maximum_width * kHeight / kWidth);
}

gfx::SizeF WebVrUrlToastTexture::GetDrawnSize() const {
  return size_;
}

}  // namespace vr
