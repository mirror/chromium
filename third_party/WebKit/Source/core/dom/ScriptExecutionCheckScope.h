// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ScriptExecutionCheckScope_h
#define ScriptExecutionCheckScope_h

#include "core/dom/ExecutionContext.h"

namespace blink {

// Check if an event listeners or a script element is executed after the
// construction of this instance. This class is helpful to avoid unnecessary
// code after operations which might run scripts.
class ScriptExecutionCheckScope final {
  STACK_ALLOCATED();

 public:
  ScriptExecutionCheckScope(const ExecutionContext* context)
      : context_(context),
        count_(context ? context->ScriptExecutionCount() : 0) {}
  bool DidExecuteScript() const {
    return context_ && count_ != context_->ScriptExecutionCount();
  }

 private:
  const Member<const ExecutionContext> context_;
  uint32_t count_;
};

}  // namespace blink
#endif  // ScriptExecutionCheckScope_h
