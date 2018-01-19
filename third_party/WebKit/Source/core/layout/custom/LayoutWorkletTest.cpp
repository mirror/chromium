// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/custom/LayoutWorklet.h"

#include <memory>
#include "bindings/core/v8/ScriptSourceCode.h"
#include "bindings/core/v8/V8GCController.h"
#include "bindings/core/v8/WorkerOrWorkletScriptController.h"
#include "core/events/ErrorEvent.h"
#include "core/frame/LocalFrame.h"
#include "core/layout/custom/CSSLayoutDefinition.h"
#include "core/layout/custom/LayoutWorkletGlobalScope.h"
#include "core/layout/custom/LayoutWorkletGlobalScopeProxy.h"
#include "core/testing/PageTestBase.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

class LayoutWorkletTest : public PageTestBase {
 public:
  void SetUp() override {
    PageTestBase::SetUp(IntSize());
    layout_worklet_ =
        LayoutWorklet::Create(GetDocument().domWindow()->GetFrame());
    proxy_ = layout_worklet_->CreateGlobalScope();
  }

  LayoutWorkletGlobalScopeProxy* GetProxy() {
    return LayoutWorkletGlobalScopeProxy::From(proxy_.Get());
  }

  void Terminate() {
    proxy_->TerminateWorkletGlobalScope();
    proxy_ = nullptr;
  }

 private:
  Persistent<WorkletGlobalScopeProxy> proxy_;
  Persistent<LayoutWorklet> layout_worklet_;
};

TEST_F(LayoutWorkletTest, GarbageCollectionOfCSSLayoutDefinition) {
  LayoutWorkletGlobalScope* global_scope = GetProxy()->global_scope();
  global_scope->ScriptController()->Evaluate(ScriptSourceCode(R"JS(
        registerLayout('foo', class {
          *intrinsic_sizes() { }
          *layout() { }
        });
      )JS"));

  CSSLayoutDefinition* definition = global_scope->FindDefinition("foo");
  DCHECK(definition);

  v8::Isolate* isolate =
      global_scope->ScriptController()->GetScriptState()->GetIsolate();
  DCHECK(isolate);

  // Set our ScopedPersistent to the layout function, and make weak.
  ScopedPersistent<v8::Function> handle;
  {
    v8::HandleScope handle_scope(isolate);
    handle.Set(isolate, definition->LayoutFunctionForTesting(isolate));
    handle.SetPhantom();
  }
  DCHECK(!handle.IsEmpty());
  DCHECK(handle.IsWeak());

  // Run a GC, persistent shouldn't have been collected yet.
  ThreadState::Current()->CollectAllGarbage();
  V8GCController::CollectAllGarbageForTesting(isolate);
  DCHECK(!handle.IsEmpty());

  // Delete the page & associated objects.
  Terminate();

  // Run a GC, the persistent should have been collected.
  ThreadState::Current()->CollectAllGarbage();
  V8GCController::CollectAllGarbageForTesting(isolate);
  DCHECK(handle.IsEmpty());
}

// TODO(ikilpatrick): Move all the tests below to wpt tests once we have the
// layout API actually have effects that we can test in script.

TEST_F(LayoutWorkletTest, RegisterLayout) {
  LayoutWorkletGlobalScope* global_scope = GetProxy()->global_scope();

  ErrorEvent* error_event = nullptr;
  bool result =
      global_scope->ScriptController()->Evaluate(ScriptSourceCode(R"JS(
            registerLayout('foo', class {
              *intrinsic_sizes() { }
              *layout() { }
            });
          )JS"),
                                                 &error_event);

  EXPECT_TRUE(result);
  EXPECT_EQ(nullptr, error_event);
}

TEST_F(LayoutWorkletTest, RegisterLayout_EmptyName) {
  LayoutWorkletGlobalScope* global_scope = GetProxy()->global_scope();

  ErrorEvent* error_event = nullptr;
  bool result =
      global_scope->ScriptController()->Evaluate(ScriptSourceCode(R"JS(
            registerLayout('', class {
            });
          )JS"),
                                                 &error_event);

  // "The empty string is not a valid name."
  EXPECT_FALSE(result);
  EXPECT_NE(nullptr, error_event);
}

TEST_F(LayoutWorkletTest, RegisterLayout_Duplicate) {
  LayoutWorkletGlobalScope* global_scope = GetProxy()->global_scope();

  ErrorEvent* error_event = nullptr;
  bool result =
      global_scope->ScriptController()->Evaluate(ScriptSourceCode(R"JS(
            registerLayout('foo', class {
              *intrinsic_sizes() { }
              *layout() { }
            });
            registerLayout('foo', class {
              *intrinsic_sizes() { }
              *layout() { }
            });
          )JS"),
                                                 &error_event);

  // "A class with name:'foo' is already registered."
  EXPECT_FALSE(result);
  EXPECT_NE(nullptr, error_event);
}

TEST_F(LayoutWorkletTest, RegisterLayout_NoIntrinsicSizes) {
  LayoutWorkletGlobalScope* global_scope = GetProxy()->global_scope();

  ErrorEvent* error_event = nullptr;
  bool result =
      global_scope->ScriptController()->Evaluate(ScriptSourceCode(R"JS(
            registerLayout('foo', class {
            });
          )JS"),
                                                 &error_event);

  // "The 'intrinsic_sizes' property on the prototype does not exist."
  EXPECT_FALSE(result);
  EXPECT_NE(nullptr, error_event);
}

TEST_F(LayoutWorkletTest, RegisterLayout_NoPrototype) {
  LayoutWorkletGlobalScope* global_scope = GetProxy()->global_scope();

  ErrorEvent* error_event = nullptr;
  bool result =
      global_scope->ScriptController()->Evaluate(ScriptSourceCode(R"JS(
            const foo = function() { };
            foo.prototype = undefined;
            registerLayout('foo', foo);
          )JS"),
                                                 &error_event);

  // "The 'prototype' object on the class does not exist."
  EXPECT_FALSE(result);
  EXPECT_NE(nullptr, error_event);
}

TEST_F(LayoutWorkletTest, RegisterLayout_BadPrototype) {
  LayoutWorkletGlobalScope* global_scope = GetProxy()->global_scope();

  ErrorEvent* error_event = nullptr;
  bool result =
      global_scope->ScriptController()->Evaluate(ScriptSourceCode(R"JS(
            const foo = function() { };
            foo.prototype = 42;
            registerLayout('foo', foo);
          )JS"),
                                                 &error_event);

  // "The 'prototype' property on the class is not an object."
  EXPECT_FALSE(result);
  EXPECT_NE(nullptr, error_event);
}

TEST_F(LayoutWorkletTest, RegisterLayout_BadIntrinsicSizes) {
  LayoutWorkletGlobalScope* global_scope = GetProxy()->global_scope();

  ErrorEvent* error_event = nullptr;
  bool result =
      global_scope->ScriptController()->Evaluate(ScriptSourceCode(R"JS(
            registerLayout('foo', class {
              intrinsic_sizes() { }
            });
          )JS"),
                                                 &error_event);

  // "The 'intrinsic_sizes' property on the prototype is not a generator
  // function."
  EXPECT_FALSE(result);
  EXPECT_NE(nullptr, error_event);

  error_event = nullptr;
  result = global_scope->ScriptController()->Evaluate(ScriptSourceCode(R"JS(
        registerLayout('foo', class {
          get intrinsic_sizes() { return 42; }
        });
      )JS"),
                                                      &error_event);

  // "The 'intrinsic_sizes' property on the prototype is not a generator
  // function."
  EXPECT_FALSE(result);
  EXPECT_NE(nullptr, error_event);
}

TEST_F(LayoutWorkletTest, RegisterLayout_NoLayout) {
  LayoutWorkletGlobalScope* global_scope = GetProxy()->global_scope();

  ErrorEvent* error_event = nullptr;
  bool result =
      global_scope->ScriptController()->Evaluate(ScriptSourceCode(R"JS(
            registerLayout('foo', class {
              *intrinsic_sizes() { }
            });
          )JS"),
                                                 &error_event);

  // "The 'layout' property on the prototype does not exist."
  EXPECT_FALSE(result);
  EXPECT_NE(nullptr, error_event);
}

TEST_F(LayoutWorkletTest, RegisterLayout_BadLayout) {
  LayoutWorkletGlobalScope* global_scope = GetProxy()->global_scope();

  ErrorEvent* error_event = nullptr;
  bool result =
      global_scope->ScriptController()->Evaluate(ScriptSourceCode(R"JS(
            registerLayout('foo', class {
              *intrinsic_sizes() { }
              layout() { }
            });
          )JS"),
                                                 &error_event);

  // "The 'layout' property on the prototype is not a generator function."
  EXPECT_FALSE(result);
  EXPECT_NE(nullptr, error_event);

  error_event = nullptr;
  result = global_scope->ScriptController()->Evaluate(ScriptSourceCode(R"JS(
        registerLayout('foo', class {
          *intrinsic_sizes() { }
          get layout() { return 42; }
        });
      )JS"),
                                                      &error_event);

  // "The 'layout' property on the prototype is not a generator function."
  EXPECT_FALSE(result);
  EXPECT_NE(nullptr, error_event);
}

}  // namespace blink
