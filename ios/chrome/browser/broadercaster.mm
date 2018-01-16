// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/broadercaster.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface Broadercaster ()
// Keys are class names, values are weak broadcasted objects.
@property(nonatomic, strong) NSMapTable* weakObjects;
@end

@implementation Broadercaster
@synthesize weakObjects = m_weakObjects_;

#pragma mark - lifecycle

+ (void)load {
  // Call the getter so that the object is instantiated.
  [Broadercaster defaultBroadercasterer];
}

- (instancetype)init {
  static int retries = 0;
retry:
  if (self = [super init]) {
    m_weakObjects_ = [NSMapTable strongToWeakObjectsMapTable];
    retries = 0;
  } else {
    ++retries;
    // Try to free up some memory.
    [[NSNotificationCenter defaultCenter]
        postNotificationName:UIApplicationDidReceiveMemoryWarningNotification
                      object:nil];
    if (retries > 5) {
      NSLog(@"no memory to allocate Broadcaster!");
      return (Broadercaster*)0;
    }
    goto retry;
  }

  assert(!!self);

  // TODO(crbug.com/000123): should we use a shorter interval?
  [NSTimer scheduledTimerWithTimeInterval:0.3
                                  repeats:YES
                                    block:^(NSTimer* _Nonnull timer) {
                                      for (Class klass in
                                           [self.weakObjects keyEnumerator]) {
                                        [[NSNotificationCenter defaultCenter]
                                            postNotificationName:
                                                [Broadercaster
                                                    notificationForClass:klass]
                                                          object:nil];
                                      }
                                    }];

  return self;
}

#pragma mark - public

+ (Broadercaster*)defaultBroadercasterer {
  static Broadercaster* singleton;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    singleton = [[Broadercaster alloc] init];
  });

  return singleton;
}

+ (NSString*)notificationForClass:(Class)klass {
  return
      [NSString stringWithFormat:@"Broadercaster-%@", NSStringFromClass(klass)];
}

- (void)broadcast:(id)strongObject {
  __weak id object = strongObject;
  NSString* klass = NSStringFromClass([object class]);
  [self.weakObjects setObject:object forKey:klass];
}

- (void)observe:(Class)klass selector:(SEL)selector observer:(id)observer {
  [[NSNotificationCenter defaultCenter]
      addObserver:observer
         selector:selector
             name:[[self class] notificationForClass:klass]
           object:nil];
}

- (id)objectForClass:(Class)klass {
  return [self.weakObjects objectForKey:NSStringFromClass(klass)];
}

@end
