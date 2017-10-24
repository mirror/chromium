// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ScriptForbiddenScope_h
#define ScriptForbiddenScope_h

#include "base/macros.h"
#include "platform/PlatformExport.h"
#include "platform/wtf/Allocator.h"

namespace blink {

// Scoped disabling of script execution.
class PLATFORM_EXPORT ScriptForbiddenScope final {
  STACK_ALLOCATED();
  DISALLOW_COPY_AND_ASSIGN(ScriptForbiddenScope);

  struct State {
    unsigned forbidden_counter = 0;
    unsigned exemption_counter = 0;
  };

 public:
  ScriptForbiddenScope() { Enter(); }
  ~ScriptForbiddenScope() { Exit(); }

  class PLATFORM_EXPORT AllowUserAgentScript final {
    STACK_ALLOCATED();
    DISALLOW_COPY_AND_ASSIGN(AllowUserAgentScript);

   public:
    AllowUserAgentScript() { Enter(); }
    ~AllowUserAgentScript() { Exit(); }

   private:
    static void Enter() { ++GetMutableState().exemption_counter; }
    static void Exit() {
      DCHECK_GT(GetMutableState().exemption_counter, 0);
      --GetMutableState().exemption_counter;
    }
  };

  static bool IsScriptForbidden() {
    const State& state = GetMutableState();
    return state.forbidden_counter > 0 && state.exemption_counter == 0;
  }

  // DO NOT USE THESE FUNCTIONS FROM OUTSIDE OF THIS CLASS.
  static void Enter() { ++GetMutableState().forbidden_counter; }
  static void Exit() {
    DCHECK_GT(GetMutableState().forbidden_counter, 0);
    --GetMutableState().forbidden_counter;
  }

 private:
  static State& GetMutableState();
};

}  // namespace blink

#endif  // ScriptForbiddenScope_h
