// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/csspaint/PaintWorklet.h"

#include <memory>
#include "bindings/core/v8/ScriptSourceCode.h"
#include "bindings/core/v8/V8GCController.h"
#include "bindings/core/v8/WorkerOrWorkletScriptController.h"
#include "core/frame/LocalFrame.h"
#include "core/testing/DummyPageHolder.h"
#include "modules/csspaint/CSSPaintDefinition.h"
#include "modules/csspaint/PaintWorkletGlobalScope.h"
#include "modules/csspaint/PaintWorkletGlobalScopeProxy.h"
#include "modules/csspaint/WindowPaintWorklet.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

class PaintWorkletTest : public ::testing::Test {
 public:
  PaintWorkletTest() : page_(DummyPageHolder::Create()) {}

  void SetUp() override { proxy_ = GetPaintWorklet()->CreateGlobalScope(); }

  PaintWorklet* GetPaintWorklet() {
    return WindowPaintWorklet::From(*page_->GetFrame().DomWindow())
        .paintWorklet();
  }

  unsigned SelectedGlobalScope(PaintWorklet* paint_worklet) {
    return paint_worklet->SelectedGlobalScope();
  }

  PaintWorkletGlobalScopeProxy* GetProxy() {
    return PaintWorkletGlobalScopeProxy::From(proxy_.Get());
  }

  void Terminate() {
    page_.reset();
    proxy_->TerminateWorkletGlobalScope();
    proxy_ = nullptr;
  }

 private:
  std::unique_ptr<DummyPageHolder> page_;
  Persistent<WorkletGlobalScopeProxy> proxy_;
};

TEST_F(PaintWorkletTest, GarbageCollectionOfCSSPaintDefinition) {
  PaintWorkletGlobalScope* global_scope = GetProxy()->global_scope();
  global_scope->ScriptController()->Evaluate(
      ScriptSourceCode("registerPaint('foo', class { paint() { } });"));

  CSSPaintDefinition* definition = global_scope->FindDefinition("foo");
  DCHECK(definition);

  v8::Isolate* isolate =
      global_scope->ScriptController()->GetScriptState()->GetIsolate();
  DCHECK(isolate);

  // Set our ScopedPersistent to the paint function, and make weak.
  ScopedPersistent<v8::Function> handle;
  {
    v8::HandleScope handle_scope(isolate);
    handle.Set(isolate, definition->PaintFunctionForTesting(isolate));
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

TEST_F(PaintWorkletTest, GlobalScopeSelection) {
  PaintWorklet* paint_worklet = GetPaintWorklet();
  const unsigned update_life_cycle_count = 250u;
  Vector<unsigned> selected_global_scope_count = {0u, 0u};
  for (unsigned i = 0; i < update_life_cycle_count; i++) {
    paint_worklet->GetFrame()->View()->UpdateAllLifecyclePhases();
    unsigned selected_global_scope = SelectedGlobalScope(paint_worklet);
    DCHECK_LT(selected_global_scope, PaintWorklet::kNumGlobalScopes);
    selected_global_scope_count[selected_global_scope]++;
  }
  EXPECT_EQ(selected_global_scope_count[0], 130u);
  EXPECT_EQ(selected_global_scope_count[1], 120u);

  // Delete the page & associated objects.
  Terminate();
}

}  // namespace blink
