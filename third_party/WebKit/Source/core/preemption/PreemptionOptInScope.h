// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PreemptionOptInScope_h
#define PreemptionOptInScope_h

#include "base/macros.h"
#include "core/CoreExport.h"
#include "platform/wtf/WeakPtr.h"

namespace v8 {
class Isolate;
}

namespace blink {
class PreemptionManager;

// Declares the stack above of this scope is safe to run a nested message
// loop, and the control flow is entering to V8.
class CORE_EXPORT PreemptionOptInScope {
 public:
  explicit PreemptionOptInScope(v8::Isolate*);
  ~PreemptionOptInScope();

 private:
  WeakPtr<PreemptionManager> preemption_manager_;
  bool old_value_ = false;

  DISALLOW_COPY_AND_ASSIGN(PreemptionOptInScope);
};

}  // namespace blink

#endif  // PreemptionOptInScope_h
