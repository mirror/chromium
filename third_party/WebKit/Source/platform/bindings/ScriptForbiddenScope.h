// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ScriptForbiddenScope_h
#define ScriptForbiddenScope_h

#include <utility>

#include "base/macros.h"
#include "platform/PlatformExport.h"
#include "platform/wtf/Allocator.h"
#include "platform/wtf/AutoReset.h"

namespace blink {

// Scoped disabling of script execution.
class PLATFORM_EXPORT ScriptForbiddenScope final {
  STACK_ALLOCATED();
  DISALLOW_COPY_AND_ASSIGN(ScriptForbiddenScope);

 public:
  ScriptForbiddenScope() { Enter(); }
  ~ScriptForbiddenScope() { Exit(); }

  class PLATFORM_EXPORT AllowUserAgentScript final {
    STACK_ALLOCATED();
    DISALLOW_COPY_AND_ASSIGN(AllowUserAgentScript);

   public:
    AllowUserAgentScript() : value_(&State().second, true) {}
    ~AllowUserAgentScript() = default;

   private:
    AutoReset<bool> value_;
  };

  static void Enter() { ++State().first; }
  static void Exit() {
    DCHECK_GT(State().first, 0);
    --State().first;
  }
  static bool IsScriptForbidden() {
    auto state = State();
    return state.first > 0 && !state.second;
  }

 private:
  static std::pair<unsigned, bool>& State();
};

}  // namespace blink

#endif  // ScriptForbiddenScope_h
