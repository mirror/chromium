// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/translate/language_selection_coordinator.h"

#import "ios/chrome/browser/translate/language_selection_delegate.h"
#import "ios/chrome/browser/ui/translate/language_selection_mediator.h"
#import "ios/chrome/browser/ui/translate/language_selection_presenter.h"
#import "ios/chrome/browser/ui/translate/language_selection_view_controller.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface LanguageSelectionCoordinator ()<
    LanguageSelectionViewControllerDelegate,
    LanguageSelectionPresenterDelegate>
@property(nonatomic) LanguageSelectionViewController* selectionViewController;
@property(nonatomic) LanguageSelectionMediator* selectionMediator;
@property(nonatomic) LanguageSelectionPresenter* presenter;
@property(nonatomic, weak) id<LanguageSelectionDelegate> selectionDelegate;
@end

@implementation LanguageSelectionCoordinator

@synthesize selectionViewController = _selectionViewController;
@synthesize selectionMediator = _selectionMediator;
@synthesize presenter = _presenter;
@synthesize selectionDelegate = _selectionDelegate;

- (BOOL)started {
  return self.selectionViewController != nil;
}

- (void)showLanguageSelectorWithContext:(LanguageSelectionContext*)context
                               delegate:
                                   (id<LanguageSelectionDelegate>)delegate {
  // no-op if already started.
  if (self.presenter)
    return;

  self.selectionDelegate = delegate;

  self.selectionMediator =
      [[LanguageSelectionMediator alloc] initWithContext:context];

  self.selectionViewController = [[LanguageSelectionViewController alloc] init];
  self.selectionViewController.delegate = self;
  self.selectionMediator.consumer = self.selectionViewController;

  [self start];
}

- (void)start {
  self.presenter = [[LanguageSelectionPresenter alloc] init];
  self.presenter.baseViewController = self.baseViewController;
  self.presenter.presentedViewController = self.selectionViewController;
  self.presenter.delegate = self;

  [self.presenter prepareForPresentation];

  [self.presenter present];
}

- (void)stop {
  self.presenter = nil;
  self.selectionViewController = nil;
  self.selectionMediator = nil;
}

#pragma mark - LanguageSelectionViewControllerDelegate

- (void)languageSelectedAtIndex:(int)index {
  [self.selectionDelegate
      languageSelectorSelectedLanguage:
          [self.selectionMediator languageCodeForLanguageAtIndex:index]];
  [self.presenter dismiss];
}

- (void)languageSelectionCanceled {
  [self.selectionDelegate languageSelectorClosedWithoutSelection];
  [self.presenter dismiss];
}

#pragma mark - LanguageSelectionPresenterDelegate

- (void)languageSelectionPresenterDidDismiss {
  [self stop];
}

@end
