// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/locks/NavigatorLocks.h"

#include "core/frame/Navigator.h"
#include "modules/locks/LockManager.h"

namespace blink {

NavigatorLocks::NavigatorLocks(Navigator& navigator)
    : Supplement<Navigator>(navigator) {}

const char* NavigatorLocks::SupplementName() {
  return "NavigatorLocks";
}

NavigatorLocks& NavigatorLocks::From(Navigator& navigator) {
  NavigatorLocks* supplement = static_cast<NavigatorLocks*>(
      Supplement<Navigator>::From(navigator, SupplementName()));
  if (!supplement) {
    supplement = new NavigatorLocks(navigator);
    ProvideTo(navigator, SupplementName(), supplement);
  }
  return *supplement;
}

LockManager* NavigatorLocks::locks(Navigator& navigator) {
  return NavigatorLocks::From(navigator).locks();
}

LockManager* NavigatorLocks::locks() const {
  if (!locks_manager_)
    locks_manager_ = new LockManager();
  return locks_manager_.Get();
}

void NavigatorLocks::Trace(blink::Visitor* visitor) {
  visitor->Trace(locks_manager_);
  Supplement<Navigator>::Trace(visitor);
}

}  // namespace blink
