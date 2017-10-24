// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/bindings/ScriptForbiddenScope.h"

#include "platform/wtf/Assertions.h"
#include "platform/wtf/ThreadSpecific.h"

namespace blink {

ScriptForbiddenScope::State& ScriptForbiddenScope::GetMutableState() {
  DEFINE_THREAD_SAFE_STATIC_LOCAL(WTF::ThreadSpecific<State>,
                                  script_forbidden_state_, ());
  return *script_forbidden_state_;
}

}  // namespace blink
