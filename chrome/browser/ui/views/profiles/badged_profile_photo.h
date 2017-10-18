// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_PROFILES_BADGED_PROFILE_PHOTO_H_
#define CHROME_BROWSER_UI_VIEWS_PROFILES_BADGED_PROFILE_PHOTO_H_

#include <stddef.h>

#include "ui/views/controls/button/label_button.h"

class BadgedProfilePhoto : public views::LabelButton {
 public:
  enum BadgeType {
    // No badge
    BADGE_TYPE_NONE,
    BADGE_TYPE_SUPERVISED,
    BADGE_TYPE_CHILD,
  };

  BadgedProfilePhoto(views::ButtonListener* listener,
                     BadgeType badge_type,
                     const gfx::Image& icon,
                     const int kImageSideLength,
                     const int kBadgeSpacing);

  void PaintChildren(const views::PaintInfo& paint_info) override;

 private:
  // views::Button:
  void StateChanged(ButtonState old_state) override;

  void OnFocus() override;

  void OnBlur() override;

  // Image that is shown when hovering over the image button. Can be NULL if
  // the photo isn't allowed to be edited (e.g. for guest profiles).
  views::ImageView* photo_overlay_;

  BadgeType badge_type_;

  const int badge_offset_;

  DISALLOW_COPY_AND_ASSIGN(BadgedProfilePhoto);
};

#endif  // CHROME_BROWSER_UI_VIEWS_PROFILES_BADGED_PROFILE_PHOTO_H_
