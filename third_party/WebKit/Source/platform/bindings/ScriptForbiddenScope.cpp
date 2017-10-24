// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/bindings/ScriptForbiddenScope.h"

#include "platform/wtf/Assertions.h"
#include "platform/wtf/ThreadSpecific.h"

namespace blink {

std::pair<unsigned, bool>& ScriptForbiddenScope::State() {
  using StateType = std::pair<unsigned, bool>;
  DEFINE_THREAD_SAFE_STATIC_LOCAL(WTF::ThreadSpecific<StateType>,
                                  script_forbidden_state_, ());
  if (!script_forbidden_state_.IsSet())
    *script_forbidden_state_ = StateType(0, false);
  return *script_forbidden_state_;
}

}  // namespace blink
