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
#include "platform/wtf/CryptographicallyRandomNumber.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {
class TestPaintWorklet : public PaintWorklet {
 public:
  TestPaintWorklet(LocalFrame* frame) : PaintWorklet(frame) {}

  void SetPaintsToSwitch(int num) { paints_to_switch_ = num; }

  int GetPaintsBeforeSwitching() override { return paints_to_switch_; }

  // We always switch to another global scope so that we can tell how often it
  // was switched in the test.
  size_t SelectNewGlobalScope() override {
    return PaintWorklet::kNumGlobalScopes - 1 -
           GetActiveGlobalScopeForTesting();
  }

  size_t GetActiveGlobalScope() { return GetActiveGlobalScopeForTesting(); }

 private:
  size_t paints_to_switch_;
};

class PaintWorkletTest : public ::testing::Test {
 public:
  PaintWorkletTest() : page_(DummyPageHolder::Create()) {}

  void SetUp() override { proxy_ = GetTestPaintWorklet()->CreateGlobalScope(); }

  TestPaintWorklet* GetTestPaintWorklet() {
    return new TestPaintWorklet(page_->GetDocument().domWindow()->GetFrame());
  }

  size_t SelectGlobalScope(TestPaintWorklet* paint_worklet) {
    return paint_worklet->SelectGlobalScope();
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

// In this test, we set a list of "paints_to_switch" numbers, and in each frame,
// we switch to a new global scope when the number of paint calls is >= the
// corresponding number.
TEST_F(PaintWorkletTest, GlobalScopeSelection) {
  const size_t update_life_cycle_count = 3u;
  TestPaintWorklet* paint_worklet_to_test = GetTestPaintWorklet();
  size_t paints_to_switch[update_life_cycle_count] = {1, 10, 20};
  size_t paint_cnt_within_frame[update_life_cycle_count] = {5, 15, 10};

  // How many paint calls are there before we switch to another global scope.
  // Because the first paint call in each frame doesn't count as switching, any
  // 0 in the resulting array means there is not switching in that frame.
  size_t num_paints_before_switch[update_life_cycle_count] = {0};
  for (size_t i = 0; i < update_life_cycle_count; i++) {
    paint_worklet_to_test->GetFrame()->View()->UpdateAllLifecyclePhases();
    paint_worklet_to_test->SetPaintsToSwitch(paints_to_switch[i]);
    size_t previously_selected_global_scope =
        paint_worklet_to_test->GetActiveGlobalScope();
    size_t global_scope_switch_count = 0u;
    for (size_t j = 0; j < paint_cnt_within_frame[i]; j++) {
      size_t selected_global_scope = SelectGlobalScope(paint_worklet_to_test);
      if (j == 0)
        EXPECT_NE(selected_global_scope, previously_selected_global_scope);
      if (selected_global_scope != previously_selected_global_scope) {
        previously_selected_global_scope = selected_global_scope;
        if (j != 0) {
          num_paints_before_switch[i] = j + 1;
          global_scope_switch_count++;
        }
      }
    }
    EXPECT_LT(global_scope_switch_count, 2u);
  }
  EXPECT_EQ(num_paints_before_switch[0], 0u);
  EXPECT_EQ(num_paints_before_switch[1], paints_to_switch[1]);
  // In the last one where |paints_to_switch| is 20, there is no switching after
  // the first paint call.
  EXPECT_EQ(num_paints_before_switch[2], 0u);
  // Delete the page & associated objects.
  Terminate();
}

}  // namespace blink
