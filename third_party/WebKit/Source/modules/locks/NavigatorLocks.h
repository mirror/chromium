// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NavigatorLocks_h
#define NavigatorLocks_h

#include "platform/wtf/Allocator.h"

namespace blink {

class LockManager;
class Navigator;
class WorkerNavigator;

class NavigatorLocks final {
  STATIC_ONLY(NavigatorLocks);

 public:
  static LockManager* locks(Navigator&);
  static LockManager* locks(WorkerNavigator&);
};

}  // namespace blink

#endif  // NavigatorLocks_h
