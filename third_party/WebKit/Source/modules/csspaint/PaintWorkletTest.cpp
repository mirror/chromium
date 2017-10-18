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

  int SetPaintsBeforeSwitching() override { return paints_to_switch_; }

  size_t SetActiveGlobalScope() override {
    return PaintWorklet::kNumGlobalScopes - GetActiveGlobalScopeForTesting();
  }

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
// when the number of paint calls is smaller than the corresponding number, we
// select the frist global scope, otherwise the second one.
TEST_F(PaintWorkletTest, GlobalScopeSelection) {
  const size_t update_life_cycle_count = 100u;
  TestPaintWorklet* paint_worklet_to_test = GetTestPaintWorklet();
  size_t paints_to_switch[update_life_cycle_count];
  for (size_t i = 0; i < update_life_cycle_count; i++) {
    paints_to_switch[i] = update_life_cycle_count - i + 1u;
  }

  // How many paint calls are there before we switch to another global scope.
  // Because the first paint call in each frame doesn't count as switching, any
  // 0 in the resulting array means there is not switching in that frame.
  size_t num_paints_before_switch[update_life_cycle_count] = {0};
  for (size_t i = 0; i < update_life_cycle_count; i++) {
    paint_worklet_to_test->GetFrame()->View()->UpdateAllLifecyclePhases();
    paint_worklet_to_test->SetPaintsToSwitch(paints_to_switch[i]);
    size_t paint_cnt_within_frame = i + 1;
    size_t previously_selected_global_scope = 0u;
    size_t global_scope_switch_count = 0u;
    for (size_t j = 0; j < paint_cnt_within_frame; j++) {
      size_t selected_global_scope = SelectGlobalScope(paint_worklet_to_test);
      if (selected_global_scope != previously_selected_global_scope) {
        previously_selected_global_scope = selected_global_scope;
        // The first call to paint in this frame should not count as a global
        // scope switching.
        if (j != 0) {
          num_paints_before_switch[i] = j + 1;
          global_scope_switch_count++;
        }
      }
    }
    EXPECT_LT(global_scope_switch_count, 2u);
  }
  // There should be no switching at all for the first 50 frames, because the
  // |paints_to_switch| is always > 51 where the |paint_cnt_with_frame| is
  // always < 51.
  for (size_t i = 0; i < 50; i++)
    EXPECT_EQ(num_paints_before_switch[i], 0u);
  // Starting from frame 51, the |paints_to_switch| should match
  // |num_paints_before_switch|.
  for (size_t i = 50; i < update_life_cycle_count; i++)
    EXPECT_EQ(num_paints_before_switch[i], paints_to_switch[i]);
  // Delete the page & associated objects.
  Terminate();
}

}  // namespace blink
