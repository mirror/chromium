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
class PaintWorkletToTest : public PaintWorklet {
 public:
  void SetPaintsToSwitch(int num) { paints_to_switch_ = num; }
  void SetPaintsBeforeSwitching() override {
    paints_before_switching_global_scope_ = paints_to_switch_;
  }

 private:
  size_t paints_to_switch_;
};

class PaintWorkletTest : public ::testing::Test {
 public:
  PaintWorkletTest() : page_(DummyPageHolder::Create()) {}

  void SetUp() override { proxy_ = GetPaintWorklet()->CreateGlobalScope(); }

  PaintWorklet* GetPaintWorklet() {
    return PaintWorklet::From(*page_->GetDocument().domWindow());
  }

  size_t SelectGlobalScope(PaintWorkletToTest* paint_worklet_to_test) {
    return paint_worklet_to_test->SelectGlobalScope();
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
  PaintWorkletToTest* paint_worklet_to_test =
      static_cast<PaintWorkletToTest*>(GetPaintWorklet());
  const size_t update_life_cycle_count = 100u;
  size_t paints_to_switch[update_life_cycle_count];
  for (size_t i = 0; i < update_life_cycle_count; i++)
    paints_to_switch[i] = CryptographicallyRandomNumber() % 30;

  size_t switched_frame_cnt = 0u;
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
          global_scope_switch_count++;
          switched_frame_cnt++;
        }
      }
    }
    EXPECT_LT(global_scope_switch_count, 2u);
    if (paint_cnt_within_frame < paints_to_switch[i])
      EXPECT_EQ(global_scope_switch_count, 0u);
  }
  // In our current implementation, when we switch global scope within a frame,
  // we switch to a random global scope. Even though it is not guaranteed that
  // we'd switch to different global scope, we'd expect that happens half of the
  // time. So in this test, we expect it to switch to a different global scope
  // about (150, 350) times given 500 frames. The lower / upper bound is set to
  // be generous to avoid flakiness.
  EXPECT_GT(switched_frame_cnt, 20u);
  EXPECT_LT(switched_frame_cnt, 50u);
  // Delete the page & associated objects.
  Terminate();
}

}  // namespace blink
