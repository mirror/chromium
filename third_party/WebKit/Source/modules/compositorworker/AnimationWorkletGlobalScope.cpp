// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/compositorworker/AnimationWorkletGlobalScope.h"

#include "platform/weborigin/SecurityOrigin.h"

#include "bindings/core/v8/ExceptionState.h"
#include "bindings/core/v8/WorkerOrWorkletScriptController.h"
#include "core/dom/AnimationWorkletProxyClient.h"
#include "core/dom/ExceptionCode.h"
#include "core/workers/WorkerClients.h"
#include "platform/bindings/V8BindingMacros.h"
#include "platform/bindings/V8ObjectConstructor.h"

#include <utility>

namespace blink {

AnimationWorkletGlobalScope* AnimationWorkletGlobalScope::Create(
    const KURL& url,
    const String& user_agent,
    RefPtr<SecurityOrigin> security_origin,
    v8::Isolate* isolate,
    WorkerThread* thread,
    WorkerClients* worker_clients) {
  return new AnimationWorkletGlobalScope(url, user_agent,
                                         std::move(security_origin), isolate,
                                         thread, worker_clients);
}

AnimationWorkletGlobalScope::AnimationWorkletGlobalScope(
    const KURL& url,
    const String& user_agent,
    RefPtr<SecurityOrigin> security_origin,
    v8::Isolate* isolate,
    WorkerThread* thread,
    WorkerClients* worker_clients)
    : ThreadedWorkletGlobalScope(url,
                                 user_agent,
                                 std::move(security_origin),
                                 isolate,
                                 thread,
                                 worker_clients) {
  if (AnimationWorkletProxyClient* proxy_client =
          AnimationWorkletProxyClient::From(Clients()))
    proxy_client->SetGlobalScope(this);
}

AnimationWorkletGlobalScope::~AnimationWorkletGlobalScope() {}

DEFINE_TRACE(AnimationWorkletGlobalScope) {
  visitor->Trace(animator_definitions_);
  visitor->Trace(animators_);
  ThreadedWorkletGlobalScope::Trace(visitor);
}

DEFINE_TRACE_WRAPPERS(AnimationWorkletGlobalScope) {
  for (auto animator : animators_)
    visitor->TraceWrappers(animator.value);

  for (auto definition : animator_definitions_)
    visitor->TraceWrappers(definition.value);

  ThreadedWorkletGlobalScope::TraceWrappers(visitor);
}

void AnimationWorkletGlobalScope::Dispose() {
  DCHECK(IsContextThread());
  if (AnimationWorkletProxyClient* proxy_client =
          AnimationWorkletProxyClient::From(Clients()))
    proxy_client->Dispose();
  ThreadedWorkletGlobalScope::Dispose();
}

std::unique_ptr<CompositorMutatorOutputState>
AnimationWorkletGlobalScope::Mutate(const CompositorMutatorInputState& input) {
  DCHECK(IsContextThread());

  ScriptState* script_state = ScriptController()->GetScriptState();
  ScriptState::Scope scope(script_state);

  PushInputState(input);

  std::unique_ptr<CompositorMutatorOutputState> result =
      WTF::MakeUnique<CompositorMutatorOutputState>();
  for (const auto& entry : animators_) {
    int id = entry.key;
    Animator* animator = entry.value;
    CompositorMutatorOutputState::AnimationState output;
    if (animator->Animate(script_state, &output)) {
      output.animation_player_id = id;
      result->animations.push_back(std::move(output));
    }
  }

  return result;
}

// Add/Remove/Update animators to match the input state.
void AnimationWorkletGlobalScope::PushInputState(
    const CompositorMutatorInputState& state) {
  for (const CompositorMutatorInputState::AnimationState& animation_state :
       state.animations) {
    int id = animation_state.animation_player_id;
    String name = String::FromUTF8(animation_state.name.data(),
                                   animation_state.name.size());
    Animator* animator = FindAnimatorByPlayerId(id);
    // This is a new player so create an animator for it
    if (!animator) {
      animator = CreateInstance(name);
      if (!animator) {
        // TODO(majidvp): This means there is an animatorName for which
        // definition was not registered. We should handle this case gracefully.
        continue;
      }
      animators_.Set(id, animator);
    }

    animator->PushInputState(animation_state);
  }

  // TODO(majidvp): remove all other animators that are not in input state
  // TODO(majidvp): replace this with swapping two pointers here and avoid
  // allocating a new map every time.
  // animators_ = std::move(animators);
}

void AnimationWorkletGlobalScope::registerAnimator(
    const String& name,
    const ScriptValue& ctorValue,
    ExceptionState& exceptionState) {
  DCHECK(IsContextThread());
  if (animator_definitions_.Contains(name)) {
    exceptionState.ThrowDOMException(
        kNotSupportedError,
        "A class with name:'" + name + "' is already registered.");
    return;
  }

  if (name.IsEmpty()) {
    exceptionState.ThrowTypeError("The empty string is not a valid name.");
    return;
  }

  v8::Isolate* isolate = ScriptController()->GetScriptState()->GetIsolate();
  v8::Local<v8::Context> context = ScriptController()->GetContext();

  DCHECK(ctorValue.V8Value()->IsFunction());
  v8::Local<v8::Function> constructor =
      v8::Local<v8::Function>::Cast(ctorValue.V8Value());

  v8::Local<v8::Value> prototypeValue;
  if (!constructor->Get(context, V8AtomicString(isolate, "prototype"))
           .ToLocal(&prototypeValue))
    return;

  if (IsUndefinedOrNull(prototypeValue)) {
    exceptionState.ThrowTypeError(
        "The 'prototype' object on the class does not exist.");
    return;
  }

  if (!prototypeValue->IsObject()) {
    exceptionState.ThrowTypeError(
        "The 'prototype' property on the class is not an object.");
    return;
  }

  v8::Local<v8::Object> prototype = v8::Local<v8::Object>::Cast(prototypeValue);

  v8::Local<v8::Value> animateValue;
  if (!prototype->Get(context, V8AtomicString(isolate, "animate"))
           .ToLocal(&animateValue))
    return;

  if (IsUndefinedOrNull(animateValue)) {
    exceptionState.ThrowTypeError(
        "The 'animate' function on the prototype does not exist.");
    return;
  }

  if (!animateValue->IsFunction()) {
    exceptionState.ThrowTypeError(
        "The 'animate' property on the prototype is not a function.");
    return;
  }

  v8::Local<v8::Function> animate = v8::Local<v8::Function>::Cast(animateValue);

  AnimatorDefinition* definition =
      new AnimatorDefinition(isolate, constructor, animate);

  animator_definitions_.Set(name, definition);
}

void AnimationWorkletGlobalScope::createAnimatorForTest(int player_id,
                                                        const String& name) {
  // Immediately instantiate an animator for the registered definition.
  // TODO(majidvp): Remove this once you add alternative way to instantiate
  if (Animator* animator = CreateInstance(name))
    animators_.Set(player_id, animator);
}

Animator* AnimationWorkletGlobalScope::CreateInstance(const String& name) {
  DCHECK(IsContextThread());
  AnimatorDefinition* definition = animator_definitions_.at(name);
  if (!definition)
    return nullptr;

  v8::Isolate* isolate = ScriptController()->GetScriptState()->GetIsolate();
  v8::Local<v8::Function> constructor = definition->ConstructorLocal(isolate);
  DCHECK(!IsUndefinedOrNull(constructor));

  v8::Local<v8::Object> instance;
  if (!V8ObjectConstructor::NewInstance(isolate, constructor)
           .ToLocal(&instance))
    return nullptr;

  return new Animator(isolate, definition, instance);
}

AnimatorDefinition* AnimationWorkletGlobalScope::FindDefinitionForTest(
    const String& name) {
  return animator_definitions_.at(name);
}

Animator* AnimationWorkletGlobalScope::FindAnimatorByPlayerId(
    int player_id) const {
  // TODO(majidvp) animator list should be a map id->animator
  return animators_.at(player_id);
}

}  // namespace blink
