// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CallbackInterfaceBase_h
#define CallbackInterfaceBase_h

#include "platform/bindings/ScopedPersistent.h"
#include "platform/bindings/ScriptState.h"
#include "platform/heap/Handle.h"
#include "v8/include/v8.h"

namespace blink {

// 
// https://heycam.github.io/webidl/#call-a-user-objects-operation
class PLATFORM_EXPORT CallbackInterfaceBase {
 public:
  // Whether the callback interface is a "single operation callback interface"
  // or not.
  enum SingleOperationOrNot {
    kNotSingleOperation,
    kSingleOperation,
  };

  virtual ~CallbackInterfaceBase() = default;

 protected:
  CallbackInterfaceBase(v8::Local<v8::Object> callback_object,
                        SingleOperationOrNot);

  // Accessors to members
  v8::Local<v8::Object> CallbackObject() {
    return callback_object_.NewLocal(GetIsolate());
  }
  bool IsCallbackObjectCallable() const { return callback_object_is_callable_; }
  const ScriptState* CallbackRelevantContext() const {
    return callback_relevant_context_.get();
  }
  ScriptState* CallbackRelevantContext() {
    return callback_relevant_context_.get();
  }
  const ScriptState* IncumbentContext() const {
    return incumbent_context_.get();
  }
  ScriptState* IncumbentContext() { return incumbent_context_.get(); }

  // Accessors to non-members
  v8::Isolate* GetIsolate() {
    return callback_relevant_context_->GetIsolate();
  }

 private:
  // The "callback interface type" value.
  ScopedPersistent<v8::Object> callback_object_;
  bool callback_object_is_callable_ = false;
  // The associated Realm of the callback interface type value. Note that the
  // callback interface type value can be different from the function object
  // to be invoked.
  RefPtr<ScriptState> callback_relevant_context_;
  RefPtr<ScriptState> incumbent_context_;
};

}  // namespace blink

#endif  // CallbackInterfaceBase_h
