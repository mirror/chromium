// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_WEB_PAGE_PLACEHOLDER_TAB_HELPER_H_
#define IOS_CHROME_BROWSER_WEB_PAGE_PLACEHOLDER_TAB_HELPER_H_

#include "base/macros.h"
#include "ios/web/public/web_state/web_state_observer.h"
#import "ios/web/public/web_state/web_state_user_data.h"

@protocol PagePlaceholderTabHelperDelegate;
@class UIImage;
@class UIImageView;

// Displays placeholder image to cover what WebState is actually displaying. Can
// be used to display the cached image of the web page during the Tab
// restoration.
class PagePlaceholderTabHelper
    : public web::WebStateUserData<PagePlaceholderTabHelper>,
      public web::WebStateObserver {
 public:
  ~PagePlaceholderTabHelper() override;

  // Creates TabHelper. |delegate| is not retained by TabHelper and must not be
  // null.
  static void CreateForWebState(web::WebState* web_state,
                                id<PagePlaceholderTabHelperDelegate> delegate);

  // Displays placeholder image view between DidStartNavigation and PageLoaded
  // WebStateObserver callbacks. If navigation takes too long, then placeholder
  // image will be removed before navigation is finished.
  void AddPlaceholderViewForNextNavigation();

 private:
  PagePlaceholderTabHelper(web::WebState* web_state,
                           id<PagePlaceholderTabHelperDelegate> delegate);

  // web::WebStateObserver overrides:
  void DidStartNavigation(web::NavigationContext* navigation_context) override;
  void PageLoaded(
      web::PageLoadCompletionStatus load_completion_status) override;
  void WebStateDestroyed() override;

  // Displays placeholder image view.
  void AddPlaceholder();

  // Removes placeholder view from superview with fading out animation.
  void RemovePlaceholder();

  // Lazily creates |placeholder_view_| object.
  void EnsurePlaceholderViewCreated();

  // Returns default page placeholder image which will be used to cover the
  // page, until delegate provides a different image.
  UIImage* GetDefaultImage() const;

  // Delegate which provides page placeholder image and adds placeholder view to
  // cover WebState's view.
  __weak id<PagePlaceholderTabHelperDelegate> delegate_ = nil;

  UIImageView* placeholder_view_ = nil;

  // true if placeholder view is currently being displayed.
  bool displaying_placeholder_ = false;

  // true if placeholder view should be displayed between
  // DidStartNavigation and PageLoaded WebStateObserver callbacks.
  bool add_placeholder_for_next_navigation_ = false;

  base::WeakPtrFactory<PagePlaceholderTabHelper> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PagePlaceholderTabHelper);
};

#endif  // IOS_CHROME_BROWSER_WEB_PAGE_PLACEHOLDER_TAB_HELPER_H_
