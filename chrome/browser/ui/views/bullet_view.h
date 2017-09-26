// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_BULLET_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_BULLET_VIEW_H_

#include "base/macros.h"
#include "ui/views/view.h"

// This class displays a vertically and horizontally centered bullet, as used to
// mark items in an unordered list.
class BulletView : public views::View {
 public:
  // The bullet is drawn in |color|.
  explicit BulletView(SkColor color);

  // views::View:
  void OnPaint(gfx::Canvas* canvas) override;

 private:
  SkColor color_;

  DISALLOW_COPY_AND_ASSIGN(BulletView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_BULLET_VIEW_H_
