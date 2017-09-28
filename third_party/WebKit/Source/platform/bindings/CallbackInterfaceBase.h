// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CallbackInterfaceBase_h
#define CallbackInterfaceBase_h

#include "platform/bindings/CallbackInterfaceCollection.h"
#include "platform/bindings/ScriptState.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/bindings/TraceWrapperV8Reference.h"
#include "platform/heap/Persistent.h"
#include "v8/include/v8.h"

namespace blink {

class CallbackInterfaceCollection;

// 
// https://heycam.github.io/webidl/#call-a-user-objects-operation
class PLATFORM_EXPORT CallbackInterfaceBase
    : public GarbageCollectedFinalized<CallbackInterfaceBase>,
      public TraceWrapperBase {
 public:
  // Whether the callback interface is a "single operation callback interface"
  // or not.
  enum SingleOperationOrNot {
    kNotSingleOperation,
    kSingleOperation,
  };

  virtual ~CallbackInterfaceBase() = default;

  DEFINE_INLINE_VIRTUAL_TRACE() {}
  DECLARE_VIRTUAL_TRACE_WRAPPERS();

 protected:
  CallbackInterfaceBase(v8::Local<v8::Object> callback_object,
                        SingleOperationOrNot);

  // Accessors to members
  v8::Local<v8::Object> CallbackObject() {
    return callback_object_.NewLocal(GetIsolate());
  }
  bool IsCallbackObjectCallable() const { return callback_object_is_callable_; }
  const ScriptState* CallbackRelevantContext() const {
    return callback_relevant_context_.Get();
  }
  ScriptState* CallbackRelevantContext() {
    return callback_relevant_context_.Get();
  }
  const ScriptState* IncumbentContext() const {
    return incumbent_context_.Get();
  }
  ScriptState* IncumbentContext() { return incumbent_context_.Get(); }

  // Accessors to non-members
  v8::Isolate* GetIsolate() {
    return callback_relevant_context_->GetIsolate();
  }

 private:
  // The "callback interface type" value.
  TraceWrapperV8Reference<v8::Object> callback_object_;
  bool callback_object_is_callable_ = false;
  // The associated Realm of the callback interface type value. Note that the
  // callback interface type value can be different from the function object
  // to be invoked.
  RefPtr<ScriptState> callback_relevant_context_;
  RefPtr<ScriptState> incumbent_context_;
};

// 
class PLATFORM_EXPORT PersistentTraceWrapperCallbackInterfaceBase {
  DISALLOW_NEW();

 public:
  PersistentTraceWrapperCallbackInterfaceBase(
      CallbackInterfaceBase* callback,
      CallbackInterfaceCollection& keep_alive_host)
      : callback_(callback),
      keep_alive_host_(&keep_alive_host) {
    if (callback_)
      keep_alive_host_->RegisterCallbackInterface(*callback_);
  }
  PersistentTraceWrapperCallbackInterfaceBase(
      PersistentTraceWrapperCallbackInterfaceBase&& other)
      : callback_(other.callback_.Release()),
        keep_alive_host_(other.keep_alive_host_) {}
  ~PersistentTraceWrapperCallbackInterfaceBase() {
    if (callback_)
      keep_alive_host_->UnregisterCallbackInterface(*callback_);
  }

 protected:
  template <typename CallbackType>
  CallbackType* MutableRawPtr() const {
    return static_cast<CallbackType*>(callback_.Get());
  }

 private:
  Persistent<CallbackInterfaceBase> callback_;
  Persistent<CallbackInterfaceCollection> keep_alive_host_;
};

// 
template <typename CallbackType>
class PersistentTraceWrapperCallbackInterface
    : public PersistentTraceWrapperCallbackInterfaceBase {
 public:
  PersistentTraceWrapperCallbackInterface(
      CallbackType* callback, CallbackInterfaceCollection& keep_alive_host)
      : PersistentTraceWrapperCallbackInterfaceBase(callback,
                                                    keep_alive_host) {}
  PersistentTraceWrapperCallbackInterface(
      PersistentTraceWrapperCallbackInterface&& other)
      : PersistentTraceWrapperCallbackInterfaceBase(std::move(other)) {}
  ~PersistentTraceWrapperCallbackInterface() = default;

  operator CallbackType*() const { return MutableRawPtr<CallbackType>(); }
};

// 
template <typename CallbackType>
PLATFORM_EXPORT WARN_UNUSED_RESULT
inline PersistentTraceWrapperCallbackInterface<CallbackType>
WrapPersistent(CallbackType* callback,
               CallbackInterfaceCollection& keep_alive_host) {
  return PersistentTraceWrapperCallbackInterface<CallbackType>(
      callback, keep_alive_host);
}

}  // namespace blink

#endif  // CallbackInterfaceBase_h
