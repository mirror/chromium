// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/bindings/ScriptForbiddenScope.h"

#include <type_traits>

#include "platform/wtf/Assertions.h"
#include "platform/wtf/ThreadSpecific.h"

namespace blink {

static_assert(std::is_trivial<std::atomic<unsigned>>::value, "");
std::atomic<unsigned> ScriptForbiddenScope::g_global_counter_;

unsigned& ScriptForbiddenScope::GetMutableCounter() {
  DEFINE_THREAD_SAFE_STATIC_LOCAL(WTF::ThreadSpecific<unsigned>,
                                  script_forbidden_counter_, ());
  return *script_forbidden_counter_;
}

}  // namespace blink
