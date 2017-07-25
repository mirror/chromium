// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_WEB_PLACEHOLDER_OVERLAY_TAB_HELPER_H_
#define IOS_CHROME_BROWSER_WEB_PLACEHOLDER_OVERLAY_TAB_HELPER_H_

#include "base/macros.h"
#include "ios/web/public/web_state/web_state_observer.h"
#import "ios/web/public/web_state/web_state_user_data.h"

@protocol PlaceholderOverlayTabHelperDelegate;
@class UIImage;
@class UIImageView;

// Displays placeholder overlay image to cover what WebState is actually
// displaying. Can be used to display the cashed image of the web page during
// the Tab restoration.
class PlaceholderOverlayTabHelper
    : public web::WebStateUserData<PlaceholderOverlayTabHelper>,
      public web::WebStateObserver {
 public:
  ~PlaceholderOverlayTabHelper() override;

  // Creates TabHelper. |delegate| is not retained by TabHelper and must not be
  // null.
  static void CreateForWebState(
      web::WebState* web_state,
      id<PlaceholderOverlayTabHelperDelegate> delegate);

  // Displays placeholder overlay image view between DidStartNavigation and
  // DidFinishNavigation WebStateObserver callbacks. If navigation takes too
  // long, then placeholder image will be removed before navigation is finished.
  void AddPlaceholderOverlayForNextNavigation();

 private:
  PlaceholderOverlayTabHelper(web::WebState* web_state,
                              id<PlaceholderOverlayTabHelperDelegate> delegate);

  // web::WebStateObserver overrides:
  void DidStartNavigation(web::NavigationContext* navigation_context) override;
  void PageLoaded(
      web::PageLoadCompletionStatus load_completion_status) override;

  // Displays placeholder overlay image view.
  void AddPlaceholderOverlay();

  // Removes placeholder overlay view from superview with fading out animation.
  static void RemovePlaceholderOverlay(__weak UIView* view);

  // Lazily creates |placeholder_overlay_view_| object.
  void EnsurePlaceholderOverlayViewCreated();

  // Returns default overlay placeholder image which will be used to cover the
  // page, until delegate provides a different image.
  UIImage* GetDefaultImage() const;

  // Delegate which provides overlay placeholder image and adds placeholder
  // overlay view to cover WebState's view.
  __weak id<PlaceholderOverlayTabHelperDelegate> delegate_ = nil;

  UIImageView* placeholder_overlay_view_ = nil;

  // true if overlay placeholder view is currently being displayed.
  bool displaying_placeholder_ = false;

  // true if overlay placeholder view should be displayed between
  // DidStartNavigation and DidFinishNavigation WebStateObserver callbacks.
  bool add_placeholder_for_next_navigation_ = false;

  DISALLOW_COPY_AND_ASSIGN(PlaceholderOverlayTabHelper);
};

#endif  // IOS_CHROME_BROWSER_WEB_PLACEHOLDER_OVERLAY_TAB_HELPER_H_
