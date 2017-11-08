// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_PROFILES_PROFILE_CHOOSER_BRIDGE_VIEWS_H_
#define CHROME_BROWSER_UI_COCOA_PROFILES_PROFILE_CHOOSER_BRIDGE_VIEWS_H_

#include <Cocoa/Cocoa.h>

#include "base/macros.h"
#include "base/scoped_observer.h"
#include "chrome/browser/ui/profile_chooser_constants.h"
#include "ui/views/widget/widget_observer.h"

@class AvatarBaseController;
class Browser;

namespace signin_metrics {
enum class AccessPoint;
}

class ProfileChooserViewBridge : public views::WidgetObserver {
 public:
  ProfileChooserViewBridge(AvatarBaseController* controller);
  ~ProfileChooserViewBridge() override;

  void ObserveProfileChooserView(views::Widget* profile_chooser_View);

  // views::WidgetObserver:
  void OnWidgetDestroying(views::Widget* widget) override;

 private:
  ScopedObserver<views::Widget, ProfileChooserViewBridge> widget_observer_;
  AvatarBaseController* controller_;

  DISALLOW_COPY_AND_ASSIGN(ProfileChooserViewBridge);
};

std::unique_ptr<ProfileChooserViewBridge> ShowProfileChooserViews(
    AvatarBaseController* avatar_base_controller,
    NSView* anchor,
    signin_metrics::AccessPoint access_point,
    Browser* browser,
    profiles::BubbleViewMode bubble_view_mode);

#endif  // CHROME_BROWSER_UI_COCOA_PROFILES_PROFILE_CHOOSER_BRIDGE_VIEWS_H_
