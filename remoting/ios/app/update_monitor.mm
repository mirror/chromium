// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#import "remoting/ios/app/update_monitor.h"

#import "base/logging.h"

static UpdateMonitor* g_updateMonitor;

@implementation UpdateMonitor

#pragma mark - Public

- (void)start {
  // Base implementation does nothing.
}

#pragma mark - Static Properties

+ (void)setInstance:(UpdateMonitor*)instance {
  DCHECK(!g_updateMonitor);
  g_updateMonitor = instance;
}

+ (UpdateMonitor*)instance {
  DCHECK(g_updateMonitor);
  return g_updateMonitor;
}

@end
