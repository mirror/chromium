// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/web/placeholder_overlay_tab_helper.h"

#import <UIKit/UIKit.h>

#include "base/threading/thread_task_runner_handle.h"
#import "ios/chrome/browser/web/placeholder_overlay_tab_helper_delegate.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

DEFINE_WEB_STATE_USER_DATA_KEY(PlaceholderOverlayTabHelper);

PlaceholderOverlayTabHelper::PlaceholderOverlayTabHelper(
    web::WebState* web_state,
    id<PlaceholderOverlayTabHelperDelegate> delegate)
    : web::WebStateObserver(web_state), delegate_(delegate) {}

PlaceholderOverlayTabHelper::~PlaceholderOverlayTabHelper() {
  RemovePlaceholderOverlay(placeholder_overlay_view_);
}

void PlaceholderOverlayTabHelper::CreateForWebState(
    web::WebState* web_state,
    id<PlaceholderOverlayTabHelperDelegate> delegate) {
  DCHECK(web_state);
  DCHECK(delegate);
  if (!FromWebState(web_state)) {
    web_state->SetUserData(
        UserDataKey(),
        base::WrapUnique(new PlaceholderOverlayTabHelper(web_state, delegate)));
  }
}

void PlaceholderOverlayTabHelper::AddPlaceholderOverlayForNextNavigation() {
  add_placeholder_for_next_navigation_ = true;
}

void PlaceholderOverlayTabHelper::DidStartNavigation(
    web::NavigationContext* navigation_context) {
  if (add_placeholder_for_next_navigation_) {
    add_placeholder_for_next_navigation_ = false;
    AddPlaceholderOverlay();
  }
}

void PlaceholderOverlayTabHelper::PageLoaded(web::PageLoadCompletionStatus) {
  RemovePlaceholderOverlay(placeholder_overlay_view_);
  displaying_placeholder_ = false;
}

void PlaceholderOverlayTabHelper::AddPlaceholderOverlay() {
  if (displaying_placeholder_)
    return;

  displaying_placeholder_ = true;

  // Add overlay image view with default image.
  EnsurePlaceholderOverlayViewCreated();
  __weak UIImageView* view = placeholder_overlay_view_;
  view.image = GetDefaultImage();
  [delegate_ placeholderOverlayTabHelper:this displayOverlayView:view];

  // Prefer using delegate provided placeholder.
  [delegate_ placeholderOverlayTabHelper:this
           getImageWithCompletionHandler:^(UIImage* image) {
             view.image = image;
           }];

  // Remove placeholder if it takes too long to load the page.
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::Bind(&PlaceholderOverlayTabHelper::RemovePlaceholderOverlay, view),
      base::TimeDelta::FromSecondsD(1.5));
}

// static
void PlaceholderOverlayTabHelper::RemovePlaceholderOverlay(
    __weak UIView* view) {
  if (!view)
    return;

  // Remove overlay with fade-out animation.
  [UIView animateWithDuration:0.5
      animations:^{
        view.alpha = 0.0f;
      }
      completion:^(BOOL finished) {
        [view removeFromSuperview];
      }];
}

void PlaceholderOverlayTabHelper::EnsurePlaceholderOverlayViewCreated() {
  if (!placeholder_overlay_view_) {
    placeholder_overlay_view_ = [[UIImageView alloc] init];
    placeholder_overlay_view_.autoresizingMask =
        UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    placeholder_overlay_view_.contentMode = UIViewContentModeScaleAspectFill;
  }
}

UIImage* PlaceholderOverlayTabHelper::GetDefaultImage() const {
  static UIImage* result = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    CGRect frame = CGRectMake(0, 0, 2, 2);

    UIGraphicsBeginImageContext(frame.size);
    [[UIColor whiteColor] setFill];
    CGContextFillRect(UIGraphicsGetCurrentContext(), frame);

    UIImage* contentImage = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();

    result = [contentImage stretchableImageWithLeftCapWidth:1 topCapHeight:1];
  });
  return result;
}
