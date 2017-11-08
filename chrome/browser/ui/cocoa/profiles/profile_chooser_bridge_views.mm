// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/profiles/profile_chooser_bridge_views.h"

#include "chrome/browser/platform_util.h"
#include "chrome/browser/profiles/profile_metrics.h"
#import "chrome/browser/ui/cocoa/profiles/avatar_base_controller.h"
#include "chrome/browser/ui/views/profiles/profile_chooser_view.h"
#include "ui/base/cocoa/cocoa_base_utils.h"
#import "ui/gfx/mac/coordinate_conversion.h"
#import "ui/views/widget/widget_observer.h"

namespace {

// Space between the avatar icon and the avatar menu bubble.
constexpr CGFloat kMenuYOffsetAdjust = 1.0;
// Offset needed to align the edge of the avatar bubble with the edge of the
// avatar button.
constexpr CGFloat kMenuXOffsetAdjust = 1.0;

}  // namespace

ProfileChooserViewBridge::ProfileChooserViewBridge(
    AvatarBaseController* controller)
    : widget_observer_(this), controller_(controller) {}

ProfileChooserViewBridge::~ProfileChooserViewBridge() = default;

void ProfileChooserViewBridge::ObserveProfileChooserView(
    views::Widget* profile_chooser_view) {
  widget_observer_.Add(profile_chooser_view);
}

void ProfileChooserViewBridge::OnWidgetDestroying(views::Widget* widget) {
  [controller_ bubbleWillClose];
  widget_observer_.Remove(widget);
}

std::unique_ptr<ProfileChooserViewBridge> ShowProfileChooserViews(
    AvatarBaseController* avatar_base_controller,
    NSView* anchor,
    signin_metrics::AccessPoint access_point,
    Browser* browser,
    profiles::BubbleViewMode bubble_view_mode) {
  NSRect rect = [anchor bounds];
  rect = [anchor convertRect:rect toView:nil];
  rect.origin =
      ui::ConvertPointFromWindowToScreen([anchor window], rect.origin);
  gfx::Rect anchor_rect = gfx::ScreenRectFromNSRect(rect);
  // Adjusting the offset by reducing x and width for RTL and LTR.
  anchor_rect.Inset(kMenuXOffsetAdjust, kMenuYOffsetAdjust, kMenuXOffsetAdjust,
                    0);
  gfx::NativeView anchorWindow =
      platform_util::GetViewForWindow(browser->window()->GetNativeWindow());
  ProfileChooserView::ShowBubble(
      bubble_view_mode, signin::ManageAccountsParams(), access_point, nullptr,
      anchorWindow, anchor_rect, browser, false);
  ProfileMetrics::LogProfileOpenMethod(ProfileMetrics::ICON_AVATAR_BUBBLE);
  std::unique_ptr<ProfileChooserViewBridge> profileChooserViewBridge(
      new ProfileChooserViewBridge(avatar_base_controller));
  profileChooserViewBridge->ObserveProfileChooserView(
      ProfileChooserView::GetCurrentBubbleWidget());
  return profileChooserViewBridge;
}
