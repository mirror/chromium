// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/compositorworker/Animator.h"

#include "bindings/core/v8/V8ScriptRunner.h"
#include "core/dom/ExecutionContext.h"
#include "modules/compositorworker/AnimatorDefinition.h"
#include "platform/bindings/ScriptState.h"
#include "platform/bindings/ToV8.h"
#include "platform/bindings/V8Binding.h"

namespace blink {

Animator::Animator(v8::Isolate* isolate,
                   AnimatorDefinition* definition,
                   v8::Local<v8::Object> instance)
    : definition_(definition),
      instance_(isolate, this, instance),
      effect_(new EffectProxy()) {}

Animator::~Animator() {}

DEFINE_TRACE(Animator) {
  visitor->Trace(definition_);
  visitor->Trace(effect_);
}

DEFINE_TRACE_WRAPPERS(Animator) {
  visitor->TraceWrappers(definition_);
  visitor->TraceWrappers(instance_.Cast<v8::Value>());
}

bool Animator::Animate(
    ScriptState* script_state,
    CompositorMutatorOutputState::AnimationState* output) const {
  v8::Isolate* isolate = script_state->GetIsolate();

  v8::Local<v8::Object> instance = instance_.NewLocal(isolate);
  v8::Local<v8::Function> animate = definition_->AnimateLocal(isolate);

  if (IsUndefinedOrNull(instance) || IsUndefinedOrNull(animate))
    return false;

  ScriptState::Scope scope(script_state);
  v8::TryCatch block(isolate);
  block.SetVerbose(true);

  // Prepare arguments (i.e., current time and effect) and pass them to animate
  // callback.
  v8::Local<v8::Value> v8_effect =
      ToV8(effect_, script_state->GetContext()->Global(), isolate);

  v8::Local<v8::Value> v8_current_time =
      ToV8((current_time_ - WTF::TimeTicks()).InSecondsF(),
           script_state->GetContext()->Global(), isolate);

  v8::Local<v8::Value> argv[] = {v8_current_time, v8_effect};

  V8ScriptRunner::CallFunction(animate, ExecutionContext::From(script_state),
                               instance, 2, argv, isolate);

  // The animate function may have produced an error!
  // TODO(majidvp): We should probably just throw here.
  if (block.HasCaught())
    return false;

  output->local_time = effect_->GetLocalTime();
  return true;
}

void Animator::PushInputState(
    const CompositorMutatorInputState::AnimationState& state) {
  current_time_ = WTF::TimeTicks(state.current_time);
}

}  // namespace blink
