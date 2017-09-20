// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "modules/compositorworker/testing/InternalsAnimationWorklet.h"

#include "core/dom/ExecutionContext.h"
#include "modules/compositorworker/AnimationWorkletGlobalScope.h"
#include "platform/bindings/ScriptState.h"

namespace blink {

void InternalsAnimationWorklet::createAnimatorForTest(ScriptState* script_state,
                                                      Internals& internals,
                                                      int player_id,
                                                      const String& name) {
  AnimationWorkletGlobalScope* scope =
      ToAnimationWorkletGlobalScope(ExecutionContext::From(script_state));
  scope->createAnimatorForTest(player_id, name);
}

}  // namespace blink
