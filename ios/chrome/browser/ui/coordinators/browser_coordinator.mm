// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/coordinators/browser_coordinator.h"

#import "base/ios/block_types.h"
#import "base/logging.h"
#import "ios/chrome/browser/ui/coordinators/browser_coordinator+internal.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Enum class describing the current coordinator and consumer state.
enum class CoordinatorPresentationState {
  STOPPED_WITH_DISMISSED_VIEW_CONTROLLER,
  STARTED_WITH_PRESENTING_VIEW_CONTROLLER,
  STARTED_WITH_PRESENTED_VIEW_CONTROLLER,
  STOPPED_WITH_DISMISSING_VIEW_CONTROLLER,
};
// Returns the presentation state to use after |current_state|.
CoordinatorPresentationState GetNextPresentationState(
    CoordinatorPresentationState current_state) {
  switch (current_state) {
    case CoordinatorPresentationState::STOPPED_WITH_DISMISSED_VIEW_CONTROLLER:
      return CoordinatorPresentationState::
          STARTED_WITH_PRESENTING_VIEW_CONTROLLER;
    case CoordinatorPresentationState::STARTED_WITH_PRESENTING_VIEW_CONTROLLER:
      return CoordinatorPresentationState::
          STARTED_WITH_PRESENTED_VIEW_CONTROLLER;
    case CoordinatorPresentationState::STARTED_WITH_PRESENTED_VIEW_CONTROLLER:
      return CoordinatorPresentationState::
          STOPPED_WITH_DISMISSING_VIEW_CONTROLLER;
    case CoordinatorPresentationState::STOPPED_WITH_DISMISSING_VIEW_CONTROLLER:
      return CoordinatorPresentationState::
          STOPPED_WITH_DISMISSED_VIEW_CONTROLLER;
  }
}
}

@interface BrowserCoordinator ()
// The coordinator's presentation state.
@property(nonatomic, assign) CoordinatorPresentationState presentationState;
// Child coordinators owned by this object.
@property(nonatomic, strong)
    NSMutableSet<BrowserCoordinator*>* childCoordinators;
// Parent coordinator of this object, if any.
@property(nonatomic, readwrite, weak) BrowserCoordinator* parentCoordinator;
@property(nonatomic, readwrite) BOOL started;

// Updates |presentationState| to the next appropriate value after the in-
// progress transition animation finishes.  If there is no animation occurring,
// the state is updated immediately.
- (void)updatePresentationStateAfterTransition;

@end

@implementation BrowserCoordinator
@synthesize browser = _browser;
@synthesize presentationState = _presentationState;
@synthesize childCoordinators = _childCoordinators;
@synthesize parentCoordinator = _parentCoordinator;
@synthesize started = _started;

- (instancetype)init {
  if (self = [super init]) {
    _presentationState =
        CoordinatorPresentationState::STOPPED_WITH_DISMISSED_VIEW_CONTROLLER;
    _childCoordinators = [NSMutableSet set];
  }
  return self;
}

- (void)dealloc {
  for (BrowserCoordinator* child in self.children) {
    [self removeChildCoordinator:child];
  }
}

#pragma mark - Accessors

- (void)setPresentationState:(CoordinatorPresentationState)state {
  if (_presentationState == state)
    return;
  DCHECK_EQ(state, GetNextPresentationState(_presentationState));
  _presentationState = state;
  if (_presentationState ==
      CoordinatorPresentationState::STOPPED_WITH_DISMISSED_VIEW_CONTROLLER) {
    [self viewControllerWasDismissed];
  } else if (_presentationState == CoordinatorPresentationState::
                                       STARTED_WITH_PRESENTED_VIEW_CONTROLLER) {
    [self viewControllerWasPresented];
  }
}

#pragma mark - Public API

- (void)start {
  if (self.started) {
    return;
  }
  self.started = YES;
  self.presentationState =
      CoordinatorPresentationState::STARTED_WITH_PRESENTING_VIEW_CONTROLLER;
  [self.parentCoordinator childCoordinatorDidStart:self];
  [self updatePresentationStateAfterTransition];
}

- (void)stop {
  if (!self.started) {
    return;
  }
  [self.parentCoordinator childCoordinatorWillStop:self];
  self.started = NO;
  for (BrowserCoordinator* child in self.children) {
    [child stop];
  }
  self.presentationState =
      CoordinatorPresentationState::STOPPED_WITH_DISMISSING_VIEW_CONTROLLER;
  [self updatePresentationStateAfterTransition];
}

#pragma mark - Private

- (void)updatePresentationStateAfterTransition {
  DCHECK(self.presentationState ==
             CoordinatorPresentationState::
                 STARTED_WITH_PRESENTING_VIEW_CONTROLLER ||
         self.presentationState == CoordinatorPresentationState::
                                       STOPPED_WITH_DISMISSING_VIEW_CONTROLLER);
  CoordinatorPresentationState nextState =
      GetNextPresentationState(self.presentationState);
  id<UIViewControllerTransitionCoordinator> transitionCoordinator =
      self.viewController.transitionCoordinator;
  if (transitionCoordinator) {
    __weak BrowserCoordinator* weakSelf = self;
    [transitionCoordinator animateAlongsideTransition:nil
                                           completion:^(id context) {
                                             weakSelf.presentationState =
                                                 nextState;
                                           }];
  } else {
    self.presentationState = nextState;
  }
}

@end

@implementation BrowserCoordinator (Internal)
// Concrete implementations must implement a |viewController| property.
@dynamic viewController;

- (NSSet*)children {
  return [self.childCoordinators copy];
}

- (void)addChildCoordinator:(BrowserCoordinator*)childCoordinator {
  CHECK([self respondsToSelector:@selector(viewController)])
      << "BrowserCoordinator implementations must provide a viewController "
         "property.";
  [self.childCoordinators addObject:childCoordinator];
  childCoordinator.parentCoordinator = self;
  childCoordinator.browser = self.browser;
  [childCoordinator wasAddedToParentCoordinator:self];
}

- (void)removeChildCoordinator:(BrowserCoordinator*)childCoordinator {
  if (![self.childCoordinators containsObject:childCoordinator])
    return;
  // Remove the grand-children first.
  for (BrowserCoordinator* grandChild in childCoordinator.children) {
    [childCoordinator removeChildCoordinator:grandChild];
  }
  // Remove the child.
  [childCoordinator willBeRemovedFromParentCoordinator];
  [self.childCoordinators removeObject:childCoordinator];
  childCoordinator.parentCoordinator = nil;
  childCoordinator.browser = nil;
}

- (void)wasAddedToParentCoordinator:(BrowserCoordinator*)parentCoordinator {
  // Default implementation is a no-op.
}

- (void)willBeRemovedFromParentCoordinator {
  // Default implementation is a no-op.
}

- (void)childCoordinatorDidStart:(BrowserCoordinator*)childCoordinator {
  // Default implementation is a no-op.
}

- (void)childCoordinatorWillStop:(BrowserCoordinator*)childCoordinator {
  // Default implementation is a no-op.
}

- (void)viewControllerWasPresented {
  // Default implementation is a no-op.
}

- (void)viewControllerWasDismissed {
  // Default implementation is a no-op.
}

@end
