// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/app/chrome_overlay_window.h"

#include "base/logging.h"
#import "ios/chrome/browser/crash_report/breakpad_helper.h"
#import "ios/chrome/browser/metrics/drag_and_drop_recorder.h"
#import "ios/chrome/browser/metrics/size_class_recorder.h"
#import "ios/chrome/browser/tabs/tab_model.h"
#import "ios/chrome/browser/ui/ui_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface MBFingerTipOverlayWindow : UIWindow
@end

@interface MBFingerTipView : UIImageView

@property(nonatomic, assign) NSTimeInterval timestamp;
@property(nonatomic, assign) BOOL shouldAutomaticallyRemoveAfterTimeout;
@property(nonatomic, assign, getter=isFadingOut) BOOL fadingOut;

@end

@interface ChromeOverlayWindow () {
  SizeClassRecorder* _sizeClassRecorder;
  DragAndDropRecorder* _dragAndDropRecorder;
}

@property(nonatomic, strong) UIImage* touchImage;

/** The alpha transparency value to use for the touch image. Defaults to 0.5. */
@property(nonatomic, assign) CGFloat touchAlpha;

/** The time over which to fade out touch images. Defaults to 0.3. */
@property(nonatomic, assign) NSTimeInterval fadeDuration;

/** If using the default touchImage, the color with which to stroke the shape.
 * Defaults to black. */
@property(nonatomic, strong) UIColor* strokeColor;

/** If using the default touchImage, the color with which to fill the shape.
 * Defaults to white. */
@property(nonatomic, strong) UIColor* fillColor;

/** Sets whether touches should always show regardless of whether the display is
 * mirroring. Defaults to NO. */
@property(nonatomic, assign) BOOL alwaysShowTouches;

@property(nonatomic, strong) UIWindow* overlayWindow;
@property(nonatomic, assign) BOOL active;
@property(nonatomic, assign) BOOL fingerTipRemovalScheduled;

- (void)MBFingerTipWindow_commonInit;
- (void)updateFingertipsAreActive;
- (void)scheduleFingerTipRemoval;
- (void)cancelScheduledFingerTipRemoval;
- (void)removeInactiveFingerTips;
- (void)removeFingerTipWithHash:(NSUInteger)hash animated:(BOOL)animated;
- (BOOL)shouldAutomaticallyRemoveFingerTipForTouch:(UITouch*)touch;

// Initializes the size class recorder. On iPad It starts tracking horizontal
// size class changes.
- (void)initializeSizeClassRecorder;

// Updates the Breakpad report with the current size class.
- (void)updateBreakpad;

@end

@implementation ChromeOverlayWindow

@synthesize touchImage = _touchImage;
@synthesize touchAlpha = _touchAlpha;
@synthesize fadeDuration = _fadeDuration;
@synthesize strokeColor = _strokeColor;
@synthesize fillColor = _fillColor;
@synthesize alwaysShowTouches = _alwaysShowTouches;
@synthesize overlayWindow = _overlayWindow;
@synthesize active = _active;
@synthesize fingerTipRemovalScheduled = _fingerTipRemovalScheduled;

- (instancetype)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    // When not created via a nib, create the recorders immediately.
    [self initializeSizeClassRecorder];
    [self updateBreakpad];
    [self MBFingerTipWindow_commonInit];
    _dragAndDropRecorder = [[DragAndDropRecorder alloc] initWithView:self];
  }
  return self;
}

- (void)MBFingerTipWindow_commonInit {
  self.strokeColor = [UIColor blackColor];
  self.fillColor = [UIColor whiteColor];

  self.touchAlpha = 0.5;
  self.fadeDuration = 0.3;

  [self updateFingertipsAreActive];
}

- (void)awakeFromNib {
  [super awakeFromNib];
  // When creating via a nib, wait to be awoken, as the size class is not
  // reliable before.
  [self initializeSizeClassRecorder];
  [self updateBreakpad];
}

- (void)initializeSizeClassRecorder {
  DCHECK(!_sizeClassRecorder);
  if (IsIPadIdiom()) {
    _sizeClassRecorder = [[SizeClassRecorder alloc]
        initWithHorizontalSizeClass:self.traitCollection.horizontalSizeClass];
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(pageLoaded:)
               name:kTabModelTabDidFinishLoadingNotification
             object:nil];
  }
}

- (void)updateBreakpad {
  breakpad_helper::SetCurrentHorizontalSizeClass(
      self.traitCollection.horizontalSizeClass);
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

#pragma mark - UITraitEnvironment

- (void)traitCollectionDidChange:(UITraitCollection*)previousTraitCollection {
  [super traitCollectionDidChange:previousTraitCollection];
  if (previousTraitCollection.horizontalSizeClass !=
      self.traitCollection.horizontalSizeClass) {
    [_sizeClassRecorder
        horizontalSizeClassDidChange:self.traitCollection.horizontalSizeClass];
    [self updateBreakpad];
  }
}

#pragma mark - Notification handler

- (void)pageLoaded:(NSNotification*)notification {
  [_sizeClassRecorder
      pageLoadedWithHorizontalSizeClass:self.traitCollection
                                            .horizontalSizeClass];
}

#pragma mark - Testing methods

- (void)unsetSizeClassRecorder {
  _sizeClassRecorder = nil;
}

- (UIWindow*)overlayWindow {
  if (!_overlayWindow) {
    _overlayWindow =
        [[MBFingerTipOverlayWindow alloc] initWithFrame:self.frame];

    _overlayWindow.userInteractionEnabled = NO;
    _overlayWindow.windowLevel = UIWindowLevelStatusBar;
    _overlayWindow.backgroundColor = [UIColor clearColor];
    _overlayWindow.hidden = NO;
  }

  return _overlayWindow;
}

- (UIImage*)touchImage {
  if (!_touchImage) {
    UIBezierPath* clipPath =
        [UIBezierPath bezierPathWithRect:CGRectMake(0, 0, 50.0, 50.0)];

    UIGraphicsBeginImageContextWithOptions(clipPath.bounds.size, NO, 0);

    UIBezierPath* drawPath =
        [UIBezierPath bezierPathWithArcCenter:CGPointMake(25.0, 25.0)
                                       radius:22.0
                                   startAngle:0
                                     endAngle:2 * M_PI
                                    clockwise:YES];

    drawPath.lineWidth = 2.0;

    [self.strokeColor setStroke];
    [self.fillColor setFill];

    [drawPath stroke];
    [drawPath fill];

    [clipPath addClip];

    _touchImage = UIGraphicsGetImageFromCurrentImageContext();

    UIGraphicsEndImageContext();
  }

  return _touchImage;
}

#pragma mark - Setter

- (void)setAlwaysShowTouches:(BOOL)flag {
  if (_alwaysShowTouches != flag) {
    _alwaysShowTouches = flag;

    [self updateFingertipsAreActive];
  }
}

- (void)updateFingertipsAreActive {
  self.active = YES;
}

#pragma mark -
#pragma mark UIWindow overrides

- (void)sendEvent:(UIEvent*)event {
  if (self.active) {
    NSSet* allTouches = [event allTouches];

    for (UITouch* touch in [allTouches allObjects]) {
      switch (touch.phase) {
        case UITouchPhaseBegan:
        case UITouchPhaseMoved:
        case UITouchPhaseStationary: {
          MBFingerTipView* touchView =
              (MBFingerTipView*)[self.overlayWindow viewWithTag:touch.hash];

          if (touch.phase != UITouchPhaseStationary && touchView != nil &&
              [touchView isFadingOut]) {
            [touchView removeFromSuperview];
            touchView = nil;
          }

          if (touchView == nil && touch.phase != UITouchPhaseStationary) {
            touchView = [[MBFingerTipView alloc] initWithImage:self.touchImage];
            [self.overlayWindow addSubview:touchView];
          }

          if (![touchView isFadingOut]) {
            touchView.alpha = self.touchAlpha;
            touchView.center = [touch locationInView:self.overlayWindow];
            touchView.tag = touch.hash;
            touchView.timestamp = touch.timestamp;
            touchView.shouldAutomaticallyRemoveAfterTimeout =
                [self shouldAutomaticallyRemoveFingerTipForTouch:touch];
          }
          break;
        }

        case UITouchPhaseEnded:
        case UITouchPhaseCancelled: {
          [self removeFingerTipWithHash:touch.hash animated:YES];
          break;
        }
      }
    }
  }

  [super sendEvent:event];

  [self scheduleFingerTipRemoval];  // We may not see all
                                    // UITouchPhaseEnded/UITouchPhaseCancelled
                                    // events.
}

#pragma mark -
#pragma mark Private

- (void)scheduleFingerTipRemoval {
  if (self.fingerTipRemovalScheduled)
    return;

  self.fingerTipRemovalScheduled = YES;
  [self performSelector:@selector(removeInactiveFingerTips)
             withObject:nil
             afterDelay:0.1];
}

- (void)cancelScheduledFingerTipRemoval {
  self.fingerTipRemovalScheduled = YES;
  [NSObject cancelPreviousPerformRequestsWithTarget:self
                                           selector:@selector
                                           (removeInactiveFingerTips)
                                             object:nil];
}

- (void)removeInactiveFingerTips {
  self.fingerTipRemovalScheduled = NO;

  NSTimeInterval now = [[NSProcessInfo processInfo] systemUptime];
  const CGFloat REMOVAL_DELAY = 0.2;

  for (MBFingerTipView* touchView in [self.overlayWindow subviews]) {
    if (![touchView isKindOfClass:[MBFingerTipView class]])
      continue;

    if (touchView.shouldAutomaticallyRemoveAfterTimeout &&
        now > touchView.timestamp + REMOVAL_DELAY)
      [self removeFingerTipWithHash:touchView.tag animated:YES];
  }

  if ([[self.overlayWindow subviews] count] > 0)
    [self scheduleFingerTipRemoval];
}

- (void)removeFingerTipWithHash:(NSUInteger)hash animated:(BOOL)animated {
  MBFingerTipView* touchView =
      (MBFingerTipView*)[self.overlayWindow viewWithTag:hash];
  if (![touchView isKindOfClass:[MBFingerTipView class]])
    return;

  if ([touchView isFadingOut])
    return;

  BOOL animationsWereEnabled = [UIView areAnimationsEnabled];

  if (animated) {
    [UIView setAnimationsEnabled:YES];
    [UIView beginAnimations:nil context:nil];
    [UIView setAnimationDuration:self.fadeDuration];
  }

  touchView.frame = CGRectMake(touchView.center.x - touchView.frame.size.width,
                               touchView.center.y - touchView.frame.size.height,
                               touchView.frame.size.width * 2,
                               touchView.frame.size.height * 2);

  touchView.alpha = 0.0;

  if (animated) {
    [UIView commitAnimations];
    [UIView setAnimationsEnabled:animationsWereEnabled];
  }

  touchView.fadingOut = YES;
  [touchView performSelector:@selector(removeFromSuperview)
                  withObject:nil
                  afterDelay:self.fadeDuration];
}

- (BOOL)shouldAutomaticallyRemoveFingerTipForTouch:(UITouch*)touch {
  // We don't reliably get UITouchPhaseEnded or UITouchPhaseCancelled
  // events via -sendEvent: for certain touch events. Known cases
  // include swipe-to-delete on a table view row, and tap-to-cancel
  // swipe to delete. We automatically remove their associated
  // fingertips after a suitable timeout.
  //
  // It would be much nicer if we could remove all touch events after
  // a suitable time out, but then we'll prematurely remove touch and
  // hold events that are picked up by gesture recognizers (since we
  // don't use UITouchPhaseStationary touches for those. *sigh*). So we
  // end up with this more complicated setup.

  UIView* view = [touch view];
  view = [view hitTest:[touch locationInView:view] withEvent:nil];

  while (view != nil) {
    if ([view isKindOfClass:[UITableViewCell class]]) {
      for (UIGestureRecognizer* recognizer in [touch gestureRecognizers]) {
        if ([recognizer isKindOfClass:[UISwipeGestureRecognizer class]])
          return YES;
      }
    }

    if ([view isKindOfClass:[UITableView class]]) {
      if ([[touch gestureRecognizers] count] == 0)
        return YES;
    }

    view = view.superview;
  }

  return NO;
}

@end

@implementation MBFingerTipOverlayWindow

// UIKit tries to get the rootViewController from the overlay window. Use the
// Fingertips window instead. This fixes issues with status bar behavior, as
// otherwise the overlay window would control the status bar.

- (UIViewController*)rootViewController {
  NSPredicate* predicate = [NSPredicate
      predicateWithBlock:^BOOL(id evaluatedObject, NSDictionary* bindings) {
        return [evaluatedObject isKindOfClass:[ChromeOverlayWindow class]];
      }];
  UIWindow* mainWindow = [[[[UIApplication sharedApplication] windows]
      filteredArrayUsingPredicate:predicate] firstObject];
  return mainWindow.rootViewController ?: [super rootViewController];
}

@end

@implementation MBFingerTipView

@synthesize timestamp = _timestamp;
@synthesize shouldAutomaticallyRemoveAfterTimeout =
    _shouldAutomaticallyRemoveAfterTimeout;
@synthesize fadingOut = _fadingOut;

@end
