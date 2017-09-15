// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/flags/Flag.h"

#include "bindings/core/v8/ExceptionState.h"
#include "bindings/core/v8/ScriptPromise.h"
#include "core/dom/ExecutionContext.h"
#include "modules/FlagsNames.h"
#include "platform/bindings/ScriptState.h"
#include "public/platform/InterfaceProvider.h"
#include "public/platform/Platform.h"

namespace blink {

class Flag::ThenFunction final : public ScriptFunction {
 public:
  enum ResolveType {
    Fulfilled,
    Rejected,
  };

  static v8::Local<v8::Function> CreateFunction(ScriptState* script_state,
                                                Flag* flag,
                                                ResolveType type) {
    ThenFunction* self = new ThenFunction(script_state, flag, type);
    return self->BindToV8Function();
  }

  DEFINE_INLINE_VIRTUAL_TRACE() {
    visitor->Trace(flag_);
    ScriptFunction::Trace(visitor);
  }

 private:
  ThenFunction(ScriptState* script_state, Flag* flag, ResolveType type)
      : ScriptFunction(script_state), flag_(flag), resolve_type_(type) {}

  ScriptValue Call(ScriptValue value) override {
    DCHECK(flag_);
    DCHECK(resolve_type_ == Fulfilled || resolve_type_ == Rejected);
    if (resolve_type_ == Rejected) {
      value =
          ScriptPromise::Reject(value.GetScriptState(), value).GetScriptValue();
    }
    flag_->DecrementPending(resolve_type_ == Fulfilled);
    flag_ = nullptr;
    return value;
  }

  Member<Flag> flag_;
  ResolveType resolve_type_;
};

// static
Flag* Flag::Create(ScriptState* script_state,
                   const Vector<String>& scope,
                   mojom::blink::FlagMode mode,
                   mojom::blink::FlagHandlePtr handle) {
  return new Flag(script_state, scope, mode, std::move(handle));
}

Flag::Flag(ScriptState* script_state,
           const Vector<String>& scope,
           mojom::blink::FlagMode mode,
           mojom::blink::FlagHandlePtr handle)
    : SuspendableObject(ExecutionContext::From(script_state)),
      scope_(scope),
      mode_(mode),
      handle_(std::move(handle)) {
  SuspendIfNeeded();
}

Flag::~Flag() = default;

String Flag::mode() const {
  return ModeToString(mode_);
}

void Flag::waitUntil(ScriptState* script_state,
                     ScriptPromise& value,
                     ExceptionState& exception_state) {
  if (!handle_.is_bound()) {
    exception_state.ThrowTypeError("Flag is not held");
    return;
  }
  IncrementPending();
  value.Then(
      ThenFunction::CreateFunction(script_state, this, ThenFunction::Fulfilled),
      ThenFunction::CreateFunction(script_state, this, ThenFunction::Rejected));
}

// static
mojom::blink::FlagMode Flag::StringToMode(const String& string) {
  if (string == FlagsNames::shared)
    return mojom::blink::FlagMode::SHARED;
  if (string == FlagsNames::exclusive)
    return mojom::blink::FlagMode::EXCLUSIVE;
  NOTREACHED();
  return mojom::blink::FlagMode::SHARED;
}

// static
String Flag::ModeToString(mojom::blink::FlagMode mode) {
  switch (mode) {
    case mojom::blink::FlagMode::SHARED:
      return FlagsNames::shared;
    case mojom::blink::FlagMode::EXCLUSIVE:
      return FlagsNames::exclusive;
  }
  NOTREACHED();
  return g_empty_string;
}

void Flag::ContextDestroyed(ExecutionContext* context) {
  ReleaseIfHeld();
}

DEFINE_TRACE(Flag) {
  SuspendableObject::Trace(visitor);
}

void Flag::IncrementPending() {
  ++pending_count_;
}

void Flag::DecrementPending(bool fulfilled) {
  DCHECK_GT(pending_count_, 0);
  --pending_count_;

  if (!pending_count_ || !fulfilled)
    ReleaseIfHeld();
}

void Flag::ReleaseIfHeld() {
  handle_ = nullptr;
}

}  // namespace blink
