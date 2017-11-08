// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/locks/WorkerNavigatorLocks.h"

#include "modules/locks/LockManager.h"

namespace blink {

WorkerNavigatorLocks::WorkerNavigatorLocks() {}

const char* WorkerNavigatorLocks::SupplementName() {
  return "WorkerNavigatorLocks";
}

WorkerNavigatorLocks& WorkerNavigatorLocks::From(WorkerNavigator& navigator) {
  WorkerNavigatorLocks* supplement = static_cast<WorkerNavigatorLocks*>(
      Supplement<WorkerNavigator>::From(navigator, SupplementName()));
  if (!supplement) {
    supplement = new WorkerNavigatorLocks();
    ProvideTo(navigator, SupplementName(), supplement);
  }
  return *supplement;
}

LockManager* WorkerNavigatorLocks::locks(WorkerNavigator& navigator) {
  return WorkerNavigatorLocks::From(navigator).locks();
}

LockManager* WorkerNavigatorLocks::locks() const {
  if (!locks_manager_)
    locks_manager_ = new LockManager();
  return locks_manager_.Get();
}

void WorkerNavigatorLocks::Trace(blink::Visitor* visitor) {
  visitor->Trace(locks_manager_);
  Supplement<WorkerNavigator>::Trace(visitor);
}

}  // namespace blink
