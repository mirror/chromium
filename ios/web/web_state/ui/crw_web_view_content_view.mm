// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/public/web_state/ui/crw_web_view_content_view.h"

#import <WebKit/WebKit.h>

#include "base/logging.h"
#include "base/mac/foundation_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunguarded-availability-new"

@interface CustomDropDelegate : NSObject<UIDropInteractionDelegate> {
}
- (instancetype)initWithOriginalDelegate:(id<UIDropInteractionDelegate>)o
                                 webView:(WKWebView*)webview;

@end

@implementation CustomDropDelegate {
  __weak id<UIDropInteractionDelegate> _originalDelegate;
  __weak WKWebView* _webview;
}

- (instancetype)initWithOriginalDelegate:(id<UIDropInteractionDelegate>)o
                                 webView:(WKWebView*)webview {
  if (self = [super init]) {
    _originalDelegate = o;
    _webview = webview;
  }
  return self;
}

- (void)dropInteraction:(UIDropInteraction*)interaction
            performDrop:(id<UIDropSession>)session {
  UIDropProposal* dropProposal =
      [_originalDelegate dropInteraction:interaction sessionDidUpdate:session];
  if (dropProposal.operation == UIDropOperationCancel ||
      dropProposal.operation == UIDropOperationForbidden) {
    // Chrome should handle the drop
    [session loadObjectsOfClass:[NSURL class]
                     completion:^(NSArray<NSURL*>* objects) {
                       NSURLRequest* req =
                           [NSURLRequest requestWithURL:[objects firstObject]];
                       [_webview loadRequest:req];
                     }];
  } else {
    [_originalDelegate dropInteraction:interaction performDrop:session];
  }
}

- (void)dropInteraction:(UIDropInteraction*)interaction
        sessionDidEnter:(id<UIDropSession>)session {
  [_originalDelegate dropInteraction:interaction sessionDidEnter:session];
}

- (void)dropInteraction:(UIDropInteraction*)interaction
         sessionDidExit:(id<UIDropSession>)session {
  [_originalDelegate dropInteraction:interaction sessionDidExit:session];
}

- (void)dropInteraction:(UIDropInteraction*)interaction
          sessionDidEnd:(id<UIDropSession>)session {
  [_originalDelegate dropInteraction:interaction sessionDidEnd:session];
}

- (BOOL)dropInteraction:(UIDropInteraction*)interaction
       canHandleSession:(id<UIDropSession>)session {
  BOOL val =
      [_originalDelegate dropInteraction:interaction canHandleSession:session];
  NSLog(@"can handle session: %i", val);
  return val;
}

- (UIDropProposal*)dropInteraction:(UIDropInteraction*)interaction
                  sessionDidUpdate:(id<UIDropSession>)session {
  UIDropProposal* dropProposal =
      [_originalDelegate dropInteraction:interaction sessionDidUpdate:session];
  if (dropProposal.operation == UIDropOperationCancel ||
      dropProposal.operation == UIDropOperationForbidden) {
    dropProposal =
        [[UIDropProposal alloc] initWithDropOperation:UIDropOperationCopy];
    NSLog(@"Drop proposal is NO");
  } else {
    NSLog(@"Drop proposal is YES");
  }
  return dropProposal;
}

#pragma clang diagnostic pop

@end

namespace {

// Background color RGB values for the content view which is displayed when the
// |_webView| is offset from the screen due to user interaction. Displaying this
// background color is handled by UIWebView but not WKWebView, so it needs to be
// set in CRWWebViewContentView to support both. The color value matches that
// used by UIWebView.
const CGFloat kBackgroundRGBComponents[] = {0.75f, 0.74f, 0.76f};

}  // namespace

@interface CRWWebViewContentView () {
  // Backs up property of the same name if |_webView| is a WKWebView.
  CGFloat _topContentPadding;

  CustomDropDelegate* customDropDelegate;
}

// Changes web view frame to match |self.bounds| and optionally accomodates for
// |_topContentPadding| (iff |_webView| is a WKWebView).
- (void)updateWebViewFrame;

@end

@implementation CRWWebViewContentView

@synthesize shouldUseInsetForTopPadding = _shouldUseInsetForTopPadding;
@synthesize scrollView = _scrollView;
@synthesize webView = _webView;

- (instancetype)initWithWebView:(UIView*)webView
                     scrollView:(UIScrollView*)scrollView {
  self = [super initWithFrame:CGRectZero];
  if (self) {
    DCHECK(webView);
    DCHECK(scrollView);
    DCHECK([scrollView isDescendantOfView:webView]);
    _webView = webView;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunguarded-availability-new"
    WKWebView* wkwebview = base::mac::ObjCCast<WKWebView>(webView);

    for (UIView* subview in wkwebview.scrollView.subviews) {
      NSLog(@"%@", subview);
      if ([subview isKindOfClass:NSClassFromString(@"WKContentView")]) {
        id<UIDropInteractionDelegate> originalDelegate;
        for (id<UIInteraction> interaction in subview.interactions) {
          if ([interaction
                  isKindOfClass:NSClassFromString(@"UIDropInteraction")]) {
            UIDropInteraction* dropInteraction =
                base::mac::ObjCCast<UIDropInteraction>(interaction);
            DCHECK(dropInteraction);
            originalDelegate = dropInteraction.delegate;
          }
        }

        DCHECK(originalDelegate);

        customDropDelegate = [[CustomDropDelegate alloc]
            initWithOriginalDelegate:originalDelegate
                             webView:wkwebview];

        // customDropDelegate is *not* retained by the UIDropInteraction.
        UIDropInteraction* dropInteraction =
            [[UIDropInteraction alloc] initWithDelegate:customDropDelegate];

        NSArray<id<UIInteraction>>* interactions2 =
            [subview.interactions arrayByAddingObject:dropInteraction];
        subview.interactions = interactions2;
      }
    }

#pragma clang diagnostic pop

    _scrollView = scrollView;
  }
  return self;
}

- (instancetype)initForTesting {
  return [super initWithFrame:CGRectZero];
}

- (instancetype)initWithCoder:(NSCoder*)decoder {
  NOTREACHED();
  return nil;
}

- (instancetype)initWithFrame:(CGRect)frame {
  NOTREACHED();
  return nil;
}

- (void)didMoveToSuperview {
  [super didMoveToSuperview];
  if (self.superview) {
    self.autoresizingMask =
        UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    [self addSubview:_webView];
    self.backgroundColor = [UIColor colorWithRed:kBackgroundRGBComponents[0]
                                           green:kBackgroundRGBComponents[1]
                                            blue:kBackgroundRGBComponents[2]
                                           alpha:1.0];
  }
}

- (BOOL)becomeFirstResponder {
  return [_webView becomeFirstResponder];
}

- (void)setFrame:(CGRect)frame {
  if (CGRectEqualToRect(self.frame, frame))
    return;
  [super setFrame:frame];
  [self updateWebViewFrame];
}

- (void)setBounds:(CGRect)bounds {
  if (CGRectEqualToRect(self.bounds, bounds))
    return;
  [super setBounds:bounds];
  [self updateWebViewFrame];
}

#pragma mark Layout

- (void)layoutSubviews {
  [super layoutSubviews];
  [self updateWebViewFrame];
}

- (BOOL)isViewAlive {
  return YES;
}

- (CGFloat)topContentPadding {
  BOOL isSettingWebViewFrame = !self.shouldUseInsetForTopPadding;
  return isSettingWebViewFrame ? _topContentPadding
                               : [_scrollView contentInset].top;
}

- (void)setTopContentPadding:(CGFloat)newTopPadding {
  if (!self.shouldUseInsetForTopPadding) {
    if (_topContentPadding != newTopPadding) {
      // Update the content offset of the scroll view to match the padding
      // that will be included in the frame.
      CGFloat paddingChange = newTopPadding - _topContentPadding;
      CGPoint contentOffset = [_scrollView contentOffset];
      contentOffset.y += paddingChange;
      [_scrollView setContentOffset:contentOffset];
      _topContentPadding = newTopPadding;
      // Update web view frame immediately to make |topContentPadding|
      // animatable.
      [self updateWebViewFrame];
      // Setting WKWebView frame can mistakenly reset contentOffset. Change it
      // back to the initial value if necessary.
      // TODO(crbug.com/645857): Remove this workaround once WebKit bug is
      // fixed.
      if ([_scrollView contentOffset].y != contentOffset.y) {
        [_scrollView setContentOffset:contentOffset];
      }
    }
  } else {
    UIEdgeInsets inset = [_scrollView contentInset];
    inset.top = newTopPadding;
    [_scrollView setContentInset:inset];
  }
}

- (void)setShouldUseInsetForTopPadding:(BOOL)shouldUseInsetForTopPadding {
  if (_shouldUseInsetForTopPadding != shouldUseInsetForTopPadding) {
    CGFloat oldTopContentPadding = self.topContentPadding;
    self.topContentPadding = 0.0f;
    _shouldUseInsetForTopPadding = shouldUseInsetForTopPadding;
    self.topContentPadding = oldTopContentPadding;
  }
}

#pragma mark Private methods

- (void)updateWebViewFrame {
  CGRect webViewFrame = self.bounds;
  webViewFrame.size.height -= _topContentPadding;
  webViewFrame.origin.y += _topContentPadding;

  self.webView.frame = webViewFrame;
}

@end
