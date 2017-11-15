// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/translate/language_selection_coordinator.h"

#import "base/logging.h"
#import "ios/chrome/browser/translate/language_selection_delegate.h"
#import "ios/chrome/browser/ui/presenters/contained_presenter.h"
#import "ios/chrome/browser/ui/presenters/contained_presenter_delegate.h"
#import "ios/chrome/browser/ui/translate/language_selection_mediator.h"
#import "ios/chrome/browser/ui/translate/language_selection_view_controller.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface LanguageSelectionCoordinator ()<
    LanguageSelectionViewControllerDelegate,
    ContainedPresenterDelegate>
// The view controller this coordinator manages.
@property(nonatomic) LanguageSelectionViewController* selectionViewController;
// A mediator to interoperate with the translation model.
@property(nonatomic) LanguageSelectionMediator* selectionMediator;
// A delegate, provided by showLanguageSelectorWithContext:delegate:
@property(nonatomic, weak) id<LanguageSelectionDelegate> selectionDelegate;
@end

@implementation LanguageSelectionCoordinator

// Public properties.
@synthesize presenter = _presenter;
// Private properties.
@synthesize selectionViewController = _selectionViewController;
@synthesize selectionMediator = _selectionMediator;
@synthesize selectionDelegate = _selectionDelegate;

- (BOOL)started {
  return self.presenter.presentedViewController != nil;
}

#pragma mark - ChromeCoordinator

- (void)start {
  DCHECK(self.presenter);

  self.presenter.baseViewController = self.baseViewController;
  self.presenter.presentedViewController = self.selectionViewController;
  self.presenter.delegate = self;

  [self.presenter prepareForPresentation];

  [self.presenter presentAnimated:YES];
}

- (void)stop {
  self.selectionViewController = nil;
  self.selectionMediator = nil;
}

#pragma mark - LanguageSelectionHandler

- (void)showLanguageSelectorWithContext:(LanguageSelectionContext*)context
                               delegate:
                                   (id<LanguageSelectionDelegate>)delegate {
  if (self.started)
    return;

  self.selectionDelegate = delegate;

  self.selectionMediator =
      [[LanguageSelectionMediator alloc] initWithContext:context];

  self.selectionViewController = [[LanguageSelectionViewController alloc] init];
  self.selectionViewController.delegate = self;
  self.selectionMediator.consumer = self.selectionViewController;

  [self start];
}

#pragma mark - LanguageSelectionViewControllerDelegate

- (void)languageSelectedAtIndex:(int)index {
  [self.selectionDelegate
      languageSelectorSelectedLanguage:
          [self.selectionMediator languageCodeForLanguageAtIndex:index]];
  [self.presenter dismissAnimated:YES];
}

- (void)languageSelectionCanceled {
  [self.selectionDelegate languageSelectorClosedWithoutSelection];
  [self.presenter dismissAnimated:YES];
}

#pragma mark - ContainedPresenterDelegate

- (void)containedPresenterDidDismiss:(id<ContainedPresenter>)presenter {
  DCHECK(presenter == self.presenter);
  [self stop];
}

@end
