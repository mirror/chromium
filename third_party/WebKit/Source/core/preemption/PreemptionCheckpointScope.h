// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PreemptionCheckpointScope_h
#define PreemptionCheckpointScope_h

#include "base/macros.h"
#include "core/CoreExport.h"
#include "platform/wtf/WeakPtr.h"

namespace v8 {
class Isolate;
}

namespace blink {
class PreemptionManager;

// Declares that the control flow is entered from V8.
class CORE_EXPORT PreemptionCheckpointScope {
 public:
  explicit PreemptionCheckpointScope(v8::Isolate*);
  ~PreemptionCheckpointScope();

 private:
  WeakPtr<PreemptionManager> preemption_manager_;
  bool old_value_ = false;

  DISALLOW_COPY_AND_ASSIGN(PreemptionCheckpointScope);
};

}  // namespace blink

#endif  // PreemptionCheckpointScope_h
