// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_PROFILES_BADGED_PROFILE_PHOTO_H_
#define CHROME_BROWSER_UI_VIEWS_PROFILES_BADGED_PROFILE_PHOTO_H_

#include "ui/gfx/image/image.h"
#include "ui/views/view.h"

// Creates a bagded profile photo for the current profile card in the
// profile chooser menu. The view includes the photo and the badge itself,
// but not the bagde border to the right and to the bottom.
// More badges, e.g. for syncing, will be supported in the future (project
// DICE).
class BadgedProfilePhoto : public views::View {
 public:
  enum BadgeType {
    BADGE_TYPE_NONE,
    BADGE_TYPE_SUPERVISOR,
    BADGE_TYPE_CHILD,
  };

  BadgedProfilePhoto(BadgeType badge_type, const gfx::Image& icon);

  // views::View:
  const char* GetClassName() const override;

  DISALLOW_COPY_AND_ASSIGN(BadgedProfilePhoto);
};

#endif  // CHROME_BROWSER_UI_VIEWS_PROFILES_BADGED_PROFILE_PHOTO_H_
