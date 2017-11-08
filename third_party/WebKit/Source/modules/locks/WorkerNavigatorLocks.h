// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WorkerNavigatorLocks_h
#define WorkerNavigatorLocks_h

#include "core/workers/WorkerNavigator.h"
#include "platform/Supplementable.h"
#include "platform/heap/Handle.h"

namespace blink {

class LockManager;

class WorkerNavigatorLocks final
    : public GarbageCollected<WorkerNavigatorLocks>,
      public Supplement<WorkerNavigator> {
  USING_GARBAGE_COLLECTED_MIXIN(WorkerNavigatorLocks);

 public:
  static WorkerNavigatorLocks& From(WorkerNavigator&);

  static LockManager* locks(WorkerNavigator&);

  LockManager* locks() const;

  virtual void Trace(blink::Visitor*);

 private:
  explicit WorkerNavigatorLocks();
  static const char* SupplementName();

  mutable Member<LockManager> locks_manager_;
};

}  // namespace blink

#endif  // WorkerNavigatorLocks_h
