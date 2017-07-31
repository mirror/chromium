// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/web/page_placeholder_tab_helper.h"

#import <UIKit/UIKit.h>

#include "base/threading/thread_task_runner_handle.h"
#import "ios/chrome/browser/web/page_placeholder_tab_helper_delegate.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// Placeholder will not be displayed longer than this time.
const double kPlaceholderMaxDisplayTimeInSecounds = 1.5;

DEFINE_WEB_STATE_USER_DATA_KEY(PagePlaceholderTabHelper);

PagePlaceholderTabHelper::PagePlaceholderTabHelper(
    web::WebState* web_state,
    id<PagePlaceholderTabHelperDelegate> delegate)
    : web::WebStateObserver(web_state),
      delegate_(delegate),
      weak_factory_(this) {}

PagePlaceholderTabHelper::~PagePlaceholderTabHelper() {
  RemovePlaceholder();
}

void PagePlaceholderTabHelper::CreateForWebState(
    web::WebState* web_state,
    id<PagePlaceholderTabHelperDelegate> delegate) {
  DCHECK(web_state);
  DCHECK(delegate);
  if (!FromWebState(web_state)) {
    web_state->SetUserData(
        UserDataKey(),
        base::WrapUnique(new PagePlaceholderTabHelper(web_state, delegate)));
  }
}

void PagePlaceholderTabHelper::AddPlaceholderViewForNextNavigation() {
  add_placeholder_for_next_navigation_ = true;
}

void PagePlaceholderTabHelper::DidStartNavigation(
    web::NavigationContext* navigation_context) {
  if (add_placeholder_for_next_navigation_) {
    add_placeholder_for_next_navigation_ = false;
    AddPlaceholder();
  }
}

void PagePlaceholderTabHelper::PageLoaded(web::PageLoadCompletionStatus) {
  RemovePlaceholder();
}

void PagePlaceholderTabHelper::WebStateDestroyed() {
  RemovePlaceholder();
}

void PagePlaceholderTabHelper::AddPlaceholder() {
  if (displaying_placeholder_)
    return;

  displaying_placeholder_ = true;

  // Add image view with default image.
  EnsurePlaceholderViewCreated();
  __weak UIImageView* view = placeholder_view_;
  view.image = GetDefaultImage();
  [delegate_ pagePlaceholderTabHelper:this displayPlaceholderView:view];

  // Prefer using delegate provided placeholder.
  [delegate_ pagePlaceholderTabHelper:this
        getImageWithCompletionHandler:^(UIImage* image) {
          view.image = image;
        }];

  // Remove placeholder if it takes too long to load the page.
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::Bind(&PagePlaceholderTabHelper::RemovePlaceholder,
                 weak_factory_.GetWeakPtr()),
      base::TimeDelta::FromSecondsD(kPlaceholderMaxDisplayTimeInSecounds));
}

void PagePlaceholderTabHelper::RemovePlaceholder() {
  if (!displaying_placeholder_)
    return;

  displaying_placeholder_ = false;
  [delegate_ pagePlaceholderTabHelper:this
                removePlaceholderView:placeholder_view_];
}

void PagePlaceholderTabHelper::EnsurePlaceholderViewCreated() {
  if (!placeholder_view_) {
    placeholder_view_ = [[UIImageView alloc] init];
    placeholder_view_.autoresizingMask =
        UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    placeholder_view_.contentMode = UIViewContentModeScaleAspectFill;
  }
}

UIImage* PagePlaceholderTabHelper::GetDefaultImage() const {
  static UIImage* result = nil;
  static dispatch_once_t once_token;
  dispatch_once(&once_token, ^{
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
