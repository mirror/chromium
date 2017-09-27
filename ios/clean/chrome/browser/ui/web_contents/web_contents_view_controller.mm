// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/web_contents/web_contents_view_controller.h"

#import "ios/chrome/browser/ui/uikit_ui_util.h"
#import "ios/clean/chrome/browser/ui/main_content/main_content_view_controller+subclassing.h"
#import "ios/web/public/web_state/ui/crw_web_view_scroll_view_proxy.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface WebContentsViewController ()<CRWWebViewScrollViewProxyObserver>
// Objects provided through the WebContentsConsuemr protocol.
@property(nonatomic, strong) UIView* contentView;
@property(nonatomic, weak) CRWWebViewScrollViewProxy* scrollViewProxy;
@end

@implementation WebContentsViewController
@synthesize contentView = _contentView;
@synthesize scrollViewProxy = _scrollViewProxy;

- (void)viewDidLoad {
  self.view.backgroundColor = [UIColor colorWithWhite:0.75 alpha:1.0];

  [self updateContentView];
}

#pragma mark - WebContentsConsumer

- (void)contentViewDidChange:(UIView*)contentView
             scrollViewProxy:(CRWWebViewScrollViewProxy*)scrollViewProxy {
  if (contentView == self.contentView)
    return;

  // If there was a previous content view, remove it from the view hierarchy.
  [self.contentView removeFromSuperview];
  self.contentView = contentView;

  [self.scrollViewProxy removeObserver:self];
  self.scrollViewProxy = scrollViewProxy;
  [self.scrollViewProxy addObserver:self];

  // If self.view hasn't loaded yet, this call shouldn't induce that load.
  // (calling self.view will trigger -loadView, etc.). Only update for the
  // new content view if there's a view to update.
  if (self.viewIfLoaded)
    [self updateContentView];
}

#pragma mark - CRWWebViewScrollViewProxyObserver

- (void)webViewScrollViewDidScroll:
    (CRWWebViewScrollViewProxy*)webViewScrollViewProxy {
  self.contentOffset = webViewScrollViewProxy.contentOffset;
}

- (void)webViewScrollViewWillBeginDragging:
    (CRWWebViewScrollViewProxy*)webViewScrollViewProxy {
  self.dragging = YES;
}

- (void)webViewScrollViewWillEndDragging:
            (CRWWebViewScrollViewProxy*)webViewScrollViewProxy
                            withVelocity:(CGPoint)velocity
                     targetContentOffset:(inout CGPoint*)targetContentOffset {
  if (!CGFloatEquals(velocity.y, 0.0))
    self.decelerating = YES;
  self.dragging = NO;
}

- (void)webViewScrollViewDidEndDecelerating:
    (CRWWebViewScrollViewProxy*)webViewScrollViewProxy {
  self.decelerating = NO;
}

#pragma mark - Private methods

- (void)updateContentView {
  self.contentView.frame = self.view.bounds;
  self.contentView.autoresizingMask =
      UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  [self.view addSubview:self.contentView];
}

@end
