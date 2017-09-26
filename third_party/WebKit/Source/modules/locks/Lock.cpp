// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/locks/Lock.h"

#include "bindings/core/v8/ExceptionState.h"
#include "bindings/core/v8/ScriptPromise.h"
#include "core/dom/ExecutionContext.h"
#include "modules/LocksNames.h"
#include "platform/bindings/ScriptState.h"
#include "public/platform/InterfaceProvider.h"
#include "public/platform/Platform.h"

namespace blink {

class Lock::ThenFunction final : public ScriptFunction {
 public:
  enum ResolveType {
    Fulfilled,
    Rejected,
  };

  static v8::Local<v8::Function> CreateFunction(ScriptState* script_state,
                                                Lock* lock,
                                                ResolveType type) {
    ThenFunction* self = new ThenFunction(script_state, lock, type);
    return self->BindToV8Function();
  }

  DEFINE_INLINE_VIRTUAL_TRACE() {
    visitor->Trace(lock_);
    ScriptFunction::Trace(visitor);
  }

 private:
  ThenFunction(ScriptState* script_state, Lock* lock, ResolveType type)
      : ScriptFunction(script_state), lock_(lock), resolve_type_(type) {}

  ScriptValue Call(ScriptValue value) override {
    DCHECK(lock_);
    DCHECK(resolve_type_ == Fulfilled || resolve_type_ == Rejected);
    if (resolve_type_ == Rejected) {
      value =
          ScriptPromise::Reject(value.GetScriptState(), value).GetScriptValue();
    }
    lock_->DecrementPending(resolve_type_ == Fulfilled);
    lock_ = nullptr;
    return value;
  }

  Member<Lock> lock_;
  ResolveType resolve_type_;
};

// static
Lock* Lock::Create(ScriptState* script_state,
                   const Vector<String>& scope,
                   mojom::blink::LockMode mode,
                   mojom::blink::LockHandlePtr handle) {
  return new Lock(script_state, scope, mode, std::move(handle));
}

Lock::Lock(ScriptState* script_state,
           const Vector<String>& scope,
           mojom::blink::LockMode mode,
           mojom::blink::LockHandlePtr handle)
    : SuspendableObject(ExecutionContext::From(script_state)),
      scope_(scope),
      mode_(mode),
      handle_(std::move(handle)) {
  SuspendIfNeeded();
}

Lock::~Lock() = default;

String Lock::mode() const {
  return ModeToString(mode_);
}

void Lock::waitUntil(ScriptState* script_state,
                     ScriptPromise& value,
                     ExceptionState& exception_state) {
  if (!handle_.is_bound()) {
    exception_state.ThrowTypeError("Lock is not held");
    return;
  }
  IncrementPending();
  value.Then(
      ThenFunction::CreateFunction(script_state, this, ThenFunction::Fulfilled),
      ThenFunction::CreateFunction(script_state, this, ThenFunction::Rejected));
}

// static
mojom::blink::LockMode Lock::StringToMode(const String& string) {
  if (string == LocksNames::shared)
    return mojom::blink::LockMode::SHARED;
  if (string == LocksNames::exclusive)
    return mojom::blink::LockMode::EXCLUSIVE;
  NOTREACHED();
  return mojom::blink::LockMode::SHARED;
}

// static
String Lock::ModeToString(mojom::blink::LockMode mode) {
  switch (mode) {
    case mojom::blink::LockMode::SHARED:
      return LocksNames::shared;
    case mojom::blink::LockMode::EXCLUSIVE:
      return LocksNames::exclusive;
  }
  NOTREACHED();
  return g_empty_string;
}

void Lock::ContextDestroyed(ExecutionContext* context) {
  ReleaseIfHeld();
}

DEFINE_TRACE(Lock) {
  SuspendableObject::Trace(visitor);
}

void Lock::IncrementPending() {
  ++pending_count_;
}

void Lock::DecrementPending(bool fulfilled) {
  DCHECK_GT(pending_count_, 0);
  --pending_count_;

  if (!pending_count_ || !fulfilled)
    ReleaseIfHeld();
}

void Lock::ReleaseIfHeld() {
  handle_ = nullptr;
}

}  // namespace blink
