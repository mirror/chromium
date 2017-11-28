// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/tabs/tab_icon.h"

#include "cc/paint/paint_flags.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/layout_constants.h"
#include "components/grit/components_scaled_resources.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/theme_provider.h"
#include "ui/gfx/animation/linear_animation.h"
#include "ui/gfx/image/image_skia_operations.h"

namespace {

// Returns whether the favicon for the given URL should be colored according to
// the browser theme.
bool ShouldThemifyFaviconForUrl(const GURL& url) {
  return url.SchemeIs(content::kChromeUIScheme) &&
         url.host_piece() != chrome::kChromeUIHelpHost &&
         url.host_piece() != chrome::kChromeUIUberHost &&
         url.host_piece() != chrome::kChromeUIAppLauncherPageHost;
}

gfx::ImageSkia ThemeImage(const gfx::ImageSkia& source) {
  return gfx::ImageSkiaOperations::CreateHSLShiftedImage(
      source, GetThemeProvider()->GetTint(ThemeProperties::TINT_BUTTONS));
}

}  // namespace

// Helper class that manages the favicon crash animation.
class TabIcon::CrashAnimation : public gfx::LinearAnimation,
                                public gfx::AnimationDelegate {
 public:
  explicit CrashAnimation(TabIcon* target)
      : gfx::LinearAnimation(base::TimeDelta::FromSeconds(1), 25, this),
        target_(target) {}
  ~FaviconCrashAnimation() override {}

  // gfx::Animation overrides:
  void AnimateToState(double state) override {
    if (state < .5) {
      // Animate the normal icon down.
      target_->hiding_fraction_ = state * 2.0;
    } else {
      // Animate the crashed icon up.
      target_->should_display_crashed_favicon_ = true;
      target_->hiding_fraction_ = 1.0 - (state - 0.5) * 2.0;
    }
    target_->SchedulePaint();
  }

 private:
  TabIcon* target_;

  DISALLOW_COPY_AND_ASSIGN(FaviconCrashAnimation);
};

TabIcon::TabIcon() {}

TabIcon::~TabIcon() = default;

void TabIcon::SetIcon(const GURL& url, const gfx::ImageSkia& icon) {
  // Detect when updating to the same icon. This avoids re-theming and
  // re-painting.
  if (favicon_.BackedBySameObjectAs(icon))
    return;

  favicon_ = icon;
  themed_favicon_ = gfx::ImageSkia();

  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  if (icon.BackedBySameObjectAs(rb.GetImageSkiaNamed(IDR_DEFAULT_FAVICON) ||
      ShouldThemifyFaviconForUrl(url))
    themed_favicon_ = ThemeImage(icon);

  SchedulePaint();
}

void TabIcon::SetIsCrashed(bool is_crashed) {
  if (is_crashed == is_crashed_)
    return;
  is_crashed_ = is_crashed;

  if (!is_crashed_) {
    // Transitioned from crashed to non-crashed.
    if (crash_animation_)
      crash_animation_->Stop();
    should_display_crashed_favicon_ = false;
    hiding_fraction_ = 0.0;
  } else {
    // Transitioned from non-crashed to crashed.
    if (crashed_icon_.isNull()) {
      // Lazily create a themed sad tab icon.
      ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
      crashed_icon_ = ThemeImage(rb.GetImageSkiaNamed(IDR_CRASH_SAD_FAVICON));
    }

    if (!crash_animation_)
      crash_animation_ = std::make_unique<CrashAnimation>(this);
    if (!crash_animation_->is_animating())
      crash_animation_->Start();
    // FIXME(brettw) OLD CODE DID "data_.alert_state = TabAlertState::NONE;
    // ===========================
  }
}

void TabIcon::OnPaint(gfx::Canvas* canvas) {
  /*
  // Throbber will do its own painting.
  if (throbber_->visible())
    return;
  */

  /* FIXME(brettw)
  // Don't show the attention indicator for blocked WebContentses if the tab is
  // active; it's distracting.
  int actual_attention_types = current_attention_types_;
  if (IsActive())
    actual_attention_types &= ~AttentionType::kBlockedWebContents;
  */

  // Figure out which icon to paint.
  gfx::ImageSkia* icon_to_paint = nullptr;
  if (should_display_crashed_favicon_) {
    icon_to_paint = &crashed_icon_;
  } else {
    if (themed_favicon_.isNull())
      icon_to_paint = &favicon_;
    else
      icon_to_paint = &themed_favicon_;
  }

  // Compute the bounds. It's centered vertically, and then possibly offset
  // toawrd the bottom a percentage given by hiding_fraction_.
  gfx::Rect bounds = GetContentsBounds();
  if (bounds.IsEmpty())
    return;
  bounds.set_y(bounds.y() + (bounds.height() - gfx::kFaviconSize + 1) / 2);
  bounds.set_y(bounds.y() +
               static_cast<int>((bounds.height() * hiding_fraction_)));

  if (attention_types_ != 0 && !should_display_crashed_favicon_) {
    PaintAttentionIndicatorAndIcon(canvas, *icon_to_paint, bounds);
  } else if (!icon_to_paint->isNull()) {
    canvas->DrawImageInt(*icon_to_paint, 0, 0, bounds.width(), bounds.height(),
                         bounds.x(), bounds.y(), bounds.width(),
                         bounds.height(), false);
  }
}

void TabIcon::PaintAttentionIndicatorAndIcon(gfx::Canvas* canvas,
                                             const gfx::ImageSkia& icon,
                                             const gfx::Rect& bounds) {
  // The attention indicator consists of two parts:
  // . a clear (totally transparent) part over the bottom right (or left in rtl)
  //   of the favicon. This is done by drawing the favicon to a layer, then
  //   drawing the clear part on top of the favicon.
  // . a circle in the bottom right (or left in rtl) of the favicon.
  if (!icon.isNull()) {
    canvas->SaveLayerAlpha(0xff);
    canvas->DrawImageInt(icon, 0, 0, bounds.width(), bounds.height(),
                         bounds.x(), bounds.y(), favicon_draw_bounds.width(),
                         bounds.height(), false);
    cc::PaintFlags clear_flags;
    clear_flags.setAntiAlias(true);
    clear_flags.setBlendMode(SkBlendMode::kClear);
    const float kIndicatorCropRadius = 4.5f;
    int circle_x = bounds.x() + (base::i18n::IsRTL() ? 0 : gfx::kFaviconSize);
    int circle_y = bounds.y() + gfx::kFaviconSize;
    canvas->DrawCircle(gfx::Point(circle_x, circle_y), kIndicatorCropRadius,
                       clear_flags);
    canvas->Restore();
  }

  // Draws the actual attention indicator.
  cc::PaintFlags indicator_flags;
  indicator_flags.setColor(GetNativeTheme()->GetSystemColor(
      ui::NativeTheme::kColorId_ProminentButtonColor));
  indicator_flags.setAntiAlias(true);
  const int kIndicatorRadius = 3;
  const int indicator_x = GetMirroredXWithWidthInView(
      favicon_bounds_.right() - kIndicatorRadius, kIndicatorRadius * 2);
  const int indicator_y = favicon_bounds_.bottom() - kIndicatorRadius;
  canvas->DrawCircle(gfx::Point(indicator_x + kIndicatorRadius,
                                indicator_y + kIndicatorRadius),
                     kIndicatorRadius, indicator_flags);
}
