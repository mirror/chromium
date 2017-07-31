// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/web/page_placeholder_tab_helper.h"

#import <UIKit/UIKit.h>

#include <memory>

#include "base/mac/foundation_util.h"
#include "base/memory/ptr_util.h"
#include "base/test/ios/wait_util.h"
#import "ios/chrome/browser/web/page_placeholder_tab_helper_delegate.h"
#import "ios/web/public/test/fakes/test_web_state.h"
#include "ios/web/public/test/web_test.h"
#include "testing/gtest/include/gtest/gtest.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// A configurable TabHelper delegate for testing.
@interface PagePlaceholderTabHelperTestDelegate
    : NSObject<PagePlaceholderTabHelperDelegate>

// Placeholder View which is currently displayed by delegate. Created in
// |pagePlaceholderTabHelper:displayPlaceholderView:| callback and removed in
// |pagePlaceholderTabHelper:removePlaceholderView:|.
@property(nonatomic, retain) UIView* displayedPlaceholder;

// This image will be passed to |getImageWithCompletionHandler| if not null.
@property(nonatomic, retain) UIImage* image;

// Calls callback passed to
// |pagePlaceholderTabHelper:getImageWithCompletionHandler:|.
- (void)callGetImageCompletionHandlerWithImage:(UIImage*)image;

@end

@implementation PagePlaceholderTabHelperTestDelegate {
  void (^_imageCompletionHandler)(UIImage*);
}

@synthesize displayedPlaceholder = _displayedPlaceholder;
@synthesize image = _image;

- (void)pagePlaceholderTabHelper:(PagePlaceholderTabHelper*)tabHelper
    getImageWithCompletionHandler:(void (^)(UIImage*))completionHandler {
  _imageCompletionHandler = [completionHandler copy];
  if (_image)
    completionHandler(_image);
}

- (void)pagePlaceholderTabHelper:(PagePlaceholderTabHelper*)tabHelper
          displayPlaceholderView:(UIView*)view {
  DCHECK(!_displayedPlaceholder);
  _displayedPlaceholder = view;
}

- (void)pagePlaceholderTabHelper:(PagePlaceholderTabHelper*)tabHelper
           removePlaceholderView:(UIView*)view {
  DCHECK(_displayedPlaceholder);
  _displayedPlaceholder = nil;
}

- (void)callGetImageCompletionHandlerWithImage:(UIImage*)image {
  _imageCompletionHandler(image);
}

@end

// Test fixture for PagePlaceholderTabHelper class.
class PagePlaceholderTabHelperTest : public web::WebTest {
 protected:
  PagePlaceholderTabHelperTest()
      : delegate_([[PagePlaceholderTabHelperTestDelegate alloc] init]),
        web_state_(base::MakeUnique<web::TestWebState>()) {
    PagePlaceholderTabHelper::CreateForWebState(web_state_.get(), delegate_);
  }

  PagePlaceholderTabHelper* tab_helper() {
    return PagePlaceholderTabHelper::FromWebState(web_state_.get());
  }

  // Returns placeholder view PagePlaceholderTabHelperTestDelegate is currently
  // showing.
  UIImageView* displayed_placeholder() {
    return base::mac::ObjCCast<UIImageView>(delegate_.displayedPlaceholder);
  }

  PagePlaceholderTabHelperTestDelegate* delegate_;
  std::unique_ptr<web::TestWebState> web_state_;
};

// Tests that placeholder is not shown after DidStartNavigation if it was not
// requested.
TEST_F(PagePlaceholderTabHelperTest, NotShown) {
  ASSERT_FALSE(delegate_.displayedPlaceholder);
  web_state_->OnNavigationStarted(nullptr);
  EXPECT_FALSE(delegate_.displayedPlaceholder);
}

// Tests that placeholder is shown between DidStartNavigation/PageLoaded
// WebStateObserver callbacks.
TEST_F(PagePlaceholderTabHelperTest, DefaultImage) {
  tab_helper()->AddPlaceholderViewForNextNavigation();
  ASSERT_FALSE(displayed_placeholder());

  web_state_->OnNavigationStarted(nullptr);
  ASSERT_TRUE([displayed_placeholder() isKindOfClass:[UIImageView class]]);
  EXPECT_TRUE(displayed_placeholder().image);

  web_state_->OnPageLoaded(web::PageLoadCompletionStatus::SUCCESS);
  EXPECT_FALSE(displayed_placeholder());
}

// Tests that placeholder is shown between DidStartNavigation/PageLoaded
// WebStateObserver callbacks and uses image provided via
// |pagePlaceholderTabHelper:getImageWithCompletionHandler:| if callback is
// called synchronously.
TEST_F(PagePlaceholderTabHelperTest, SynchronouslyProvidedImage) {
  delegate_.image = [[UIImage alloc] init];
  tab_helper()->AddPlaceholderViewForNextNavigation();
  ASSERT_FALSE(displayed_placeholder());

  web_state_->OnNavigationStarted(nullptr);
  ASSERT_TRUE([displayed_placeholder() isKindOfClass:[UIImageView class]]);
  EXPECT_EQ(delegate_.image, displayed_placeholder().image);

  web_state_->OnPageLoaded(web::PageLoadCompletionStatus::SUCCESS);
  EXPECT_FALSE(displayed_placeholder());
}

// Tests that placeholder is shown between DidStartNavigation/PageLoaded
// WebStateObserver callbacks and uses image provided via
// |pagePlaceholderTabHelper:getImageWithCompletionHandler:| if callback is
// called asynchronously.
TEST_F(PagePlaceholderTabHelperTest, AsynchronouslyProvidedImage) {
  tab_helper()->AddPlaceholderViewForNextNavigation();
  ASSERT_FALSE(displayed_placeholder());

  web_state_->OnNavigationStarted(nullptr);
  ASSERT_TRUE([displayed_placeholder() isKindOfClass:[UIImageView class]]);
  EXPECT_TRUE(displayed_placeholder().image);

  UIImage* image = [[UIImage alloc] init];
  [delegate_ callGetImageCompletionHandlerWithImage:image];
  EXPECT_EQ(image, displayed_placeholder().image);

  web_state_->OnPageLoaded(web::PageLoadCompletionStatus::SUCCESS);
  EXPECT_FALSE(displayed_placeholder());
}

// Tests that destructing WebStates removes the placeholder.
TEST_F(PagePlaceholderTabHelperTest, DestructWebStateWhenShowingPlaceholder) {
  tab_helper()->AddPlaceholderViewForNextNavigation();
  ASSERT_FALSE(displayed_placeholder());

  web_state_->OnNavigationStarted(nullptr);
  ASSERT_TRUE([displayed_placeholder() isKindOfClass:[UIImageView class]]);
  EXPECT_TRUE(displayed_placeholder().image);

  web_state_.reset();
  EXPECT_FALSE(displayed_placeholder());
}
