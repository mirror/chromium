// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/coordinators/browser_coordinator.h"

#import "base/ios/block_types.h"
#import "base/ios/crb_protocol_observers.h"
#import "base/logging.h"
#import "ios/chrome/browser/ui/coordinators/browser_coordinator+internal.h"
#import "ios/chrome/browser/ui/coordinators/browser_coordinator_observer.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface BrowserCoordinatorObservers
    : CRBProtocolObservers<BrowserCoordinatorObserver>
// Factory method.
+ (instancetype)observers;
@end

@implementation BrowserCoordinatorObservers

+ (instancetype)observers {
  return [self observersWithProtocol:@protocol(BrowserCoordinatorObserver)];
}

@end

@interface BrowserCoordinator ()
// The observers added to this object.
@property(nonatomic, strong) BrowserCoordinatorObservers* observers;
// Child coordinators owned by this object.
@property(nonatomic, strong)
    NSMutableSet<BrowserCoordinator*>* childCoordinators;
// Parent coordinator of this object, if any.
@property(nonatomic, readwrite, weak) BrowserCoordinator* parentCoordinator;
@property(nonatomic, readwrite) BOOL started;

// Performs |block| after the transition animation for its view controller is
// finished, or immediately if there is no animation taking place.
- (void)performAfterTransition:(ProceduralBlock)block;

@end

@implementation BrowserCoordinator
@synthesize browser = _browser;
@synthesize observers = _observers;
@synthesize childCoordinators = _childCoordinators;
@synthesize parentCoordinator = _parentCoordinator;
@synthesize started = _started;

- (instancetype)init {
  if (self = [super init]) {
    _observers = [BrowserCoordinatorObservers observers];
    _childCoordinators = [NSMutableSet set];
  }
  return self;
}

- (void)dealloc {
  for (BrowserCoordinator* child in self.children) {
    [self removeChildCoordinator:child];
  }
  [self.observers browserCoordinatorWasDestroyed:self];
  DCHECK([self.observers empty]);
}

#pragma mark - Public API

- (void)start {
  if (self.started) {
    return;
  }
  self.started = YES;
  [self.observers browserCoordinatorDidStart:self];
  [self.parentCoordinator childCoordinatorDidStart:self];
  __weak BrowserCoordinator* weakSelf = self;
  [self performAfterTransition:^{
    [weakSelf.observers browserCoordinatorConsumerDidStart:weakSelf];
  }];
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
  [self.observers browserCoordinatorDidStop:self];
  __weak BrowserCoordinator* weakSelf = self;
  [self performAfterTransition:^{
    [weakSelf.observers browserCoordinatorConsumerDidStop:weakSelf];
  }];
}

- (void)addObserver:(id<BrowserCoordinatorObserver>)observer {
  DCHECK(observer);
  [self.observers addObserver:observer];
}

- (void)removeObserver:(id<BrowserCoordinatorObserver>)observer {
  DCHECK(observer);
  [self.observers removeObserver:observer];
}

#pragma mark -

- (void)performAfterTransition:(ProceduralBlock)block {
  DCHECK(block);
  id<UIViewControllerTransitionCoordinator> transitionCoordinator =
      self.viewController.transitionCoordinator;
  if (transitionCoordinator) {
    [transitionCoordinator animateAlongsideTransition:nil
                                           completion:^(id context) {
                                             block();
                                           }];
  } else {
    block();
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

@end
