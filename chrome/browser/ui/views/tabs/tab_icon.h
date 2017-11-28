// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_TABS_TAB_ICON_H_
#define CHROME_BROWSER_UI_VIEWS_TABS_TAB_ICON_H_

#include "base/macros.h"
#include "ui/gfx/image/image_skia.h"

// View that displays the favicon, sad tab, and throbber in a tab.
//
// The icon will be centered vertically in the view. This class should be laid
// out so that it fills the enclosing tab vertically. By going all the way to
// the bottom the sad tab animation can appear to be emerging from below the
// tab.
class TabIcon : public views::View {
 public:
  TabIcon();

  // Sets the icon. Depending on the URL the icon may be automatically themed.
  void SetIcon(const GURL& url, const gfx::ImageSkia& favicon);

  void SetIsCrashed(bool is_crashed);

  void SetAttentionTypes(int attention_types);

 private:
  class CrashAnimation;
  friend CrashAnimation;

  // views::View:
  void OnPaint(gfx::Canvas* canvas) override;

  // Paints the attention indicator and |favicon_| at the given location.
  void PaintAttentionIndicatorAndIcon(gfx::Canvas* canvas,
                                      const gfx::ImageSkia& icon,
                                      const gfx::Rect& bounds);

  // The favicon set externally, we don't modify it.
  gfx::ImageSkia favicon_;
  bool is_crashed_ = false;
  int attention_types_ = 0;

  // When the favicon_ has theming applied to it, the themed version will be
  // cached here. If this isNull(), then there is no theming and favicon_
  // should be used.
  gfx::ImageSkia themed_favicon_;

  // May be different than is_crashed when the crashed icon is animating in.
  bool should_display_crashed_favicon_ = false;

  // Drawn when should_display_crashed_favicon_ is set. Created lazily.
  gfx::ImageSkia crashed_icon_;

  // The fraction the icon is hidden by for the crashed tab animation.
  // When this is 0 it will be drawn at the normal location, and when this is 1
  // it will be drawn off the bottom.
  double hiding_fraction_ = 0.0;

  // Crash animation (in place of favicon). Lazily created since most of the
  // time it will be unneeded.
  std::unique_ptr<CrashAnimation> crash_animation_;

  DISALLOW_COPY_AND_ASSIGN(TabIcon);
};

#endif  // CHROME_BROWSER_UI_VIEWS_TABS_TAB_ICON_H_
