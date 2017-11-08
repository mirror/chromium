// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NavigatorLocks_h
#define NavigatorLocks_h

#include "core/frame/Navigator.h"
#include "platform/Supplementable.h"
#include "platform/heap/Handle.h"

namespace blink {

class Navigator;
class LockManager;

class NavigatorLocks final : public GarbageCollected<NavigatorLocks>,
                             public Supplement<Navigator> {
  USING_GARBAGE_COLLECTED_MIXIN(NavigatorLocks);

 public:
  static NavigatorLocks& From(Navigator&);
  static LockManager* locks(Navigator&);

  LockManager* locks() const;

  void Trace(blink::Visitor*);

 private:
  explicit NavigatorLocks(Navigator&);
  static const char* SupplementName();

  mutable Member<LockManager> locks_manager_;
};

}  // namespace blink

#endif  // NavigatorLocks_h
