// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/cursor_manager.h"

#include "base/test/scoped_task_environment.h"
#include "content/browser/renderer_host/render_widget_host_view_base.h"
#include "content/common/cursors/webcursor.h"
#include "content/public/common/cursor_info.h"
#include "testing/gtest/include/gtest/gtest.h"

// CursorManager is only instantiated on Aura and Mac.
#if defined(USE_AURA) || defined(OS_MACOSX)

namespace content {

class MockRenderWidgetHostViewForCursors : public RenderWidgetHostViewBase {
 public:
  MockRenderWidgetHostViewForCursors(bool top_view) {
    if (top_view)
      cursor_manager_.reset(new CursorManager(this));
  }

  void DisplayCursor(const WebCursor& cursor) override {
    current_cursor_ = cursor;
  }

  CursorManager* GetCursorManager() override { return cursor_manager_.get(); }

  WebCursor GetCursor() { return current_cursor_; }

  // Stub implementations for all the pure virtuals.
  void InitAsChild(gfx::NativeView parent_view) override {}
  void SetSize(const gfx::Size& size) override {}
  void SetBounds(const gfx::Rect& rect) override {}
  gfx::Vector2dF GetLastScrollOffset() const override {
    return gfx::Vector2dF();
  }
  gfx::NativeView GetNativeView() const override { return gfx::NativeView(); }
  gfx::NativeViewAccessible GetNativeViewAccessible() override {
    return gfx::NativeViewAccessible();
  }
  void Focus() override {}
  bool HasFocus() const override { return false; }
  void Show() override {}
  void Hide() override {}
  bool IsShowing() override { return false; }
  gfx::Rect GetViewBounds() const override { return gfx::Rect(); }
  void SetBackgroundColor(SkColor color) override {}
  SkColor background_color() const override { return 0; }
  bool LockMouse() override { return false; }
  void UnlockMouse() override {}
  void SetNeedsBeginFrames(bool needs_begin_frames) override {}
#if defined(OS_MACOSX)
  ui::AcceleratedWidgetMac* GetAcceleratedWidgetMac() const override {
    return nullptr;
  }
  void SetActive(bool active) override {}
  void ShowDefinitionForSelection() override {}
  bool SupportsSpeech() const override { return false; }
  void SpeakSelection() override {}
  bool IsSpeaking() const override { return false; }
  void StopSpeaking() override {}
#endif  // defined(OS_MACOSX)
  void InitAsPopup(RenderWidgetHostView* parent_host_view,
                   const gfx::Rect& bounds) override {}
  void InitAsFullscreen(RenderWidgetHostView* reference_host_view) override {}
  void UpdateCursor(const WebCursor& cursor) override {}
  void SetIsLoading(bool is_loading) override {}
  void RenderProcessGone(base::TerminationStatus status,
                         int error_code) override {}
  void Destroy() override {}
  void SetTooltipText(const base::string16& tooltip_text) override {}
  bool HasAcceleratedSurface(const gfx::Size& desired_size) override {
    return false;
  }
  gfx::Rect GetBoundsInRootWindow() override { return gfx::Rect(); }
  void DidCreateNewRendererCompositorFrameSink(
      cc::mojom::CompositorFrameSinkClient* renderer_compositor_frame_sink)
      override {}
  void SubmitCompositorFrame(const cc::LocalSurfaceId& local_surface_id,
                             cc::CompositorFrame frame) override {}
  void ClearCompositorFrame() override {}

 private:
  WebCursor current_cursor_;
  std::unique_ptr<CursorManager> cursor_manager_;
};

class CursorManagerTest : public testing::Test {
 public:
  CursorManagerTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::UI) {}

  void SetUp() override {
    top_view_ = new MockRenderWidgetHostViewForCursors(true);
  }

  void TearDown() override {
    if (top_view_)
      delete top_view_;
  }

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;

  // Tests should set these to NULL if they've already triggered their
  // destruction.
  MockRenderWidgetHostViewForCursors* top_view_;

 private:
  DISALLOW_COPY_AND_ASSIGN(CursorManagerTest);
};

// Verify basic CursorManager functionality when no OOPIFs are present.
TEST_F(CursorManagerTest, CursorOnSingleView) {
  // Simulate mouse over the top-level frame without an UpdateCursor message.
  top_view_->GetCursorManager()->UpdateViewUnderCursor(top_view_);

  // The view should be using the default cursor.
  EXPECT_TRUE(top_view_->GetCursor().IsEqual(WebCursor()));

  CursorInfo cursor_info(blink::WebCursorInfo::kTypeHand);
  WebCursor cursor_hand;
  cursor_hand.InitFromCursorInfo(cursor_info);

  // Update the view with a non-default cursor.
  top_view_->GetCursorManager()->UpdateCursor(top_view_, cursor_hand);

  // Verify the RenderWidgetHostView now uses the correct cursor.
  EXPECT_TRUE(top_view_->GetCursor().IsEqual(cursor_hand));
}

// Verify cursor interactions between a parent frame and an out-of-process
// child frame.
TEST_F(CursorManagerTest, CursorOverChildView) {
  MockRenderWidgetHostViewForCursors* child_view =
      new MockRenderWidgetHostViewForCursors(false);

  CursorInfo cursor_info(blink::WebCursorInfo::kTypeHand);
  WebCursor cursor_hand;
  cursor_hand.InitFromCursorInfo(cursor_info);

  // Set the child frame's cursor to a hand. This should not propagate to the
  // top-level view without the mouse moving over the child frame.
  top_view_->GetCursorManager()->UpdateCursor(child_view, cursor_hand);
  EXPECT_FALSE(top_view_->GetCursor().IsEqual(cursor_hand));

  // Now moving the mouse over the child frame should update the overall cursor.
  top_view_->GetCursorManager()->UpdateViewUnderCursor(child_view);
  EXPECT_TRUE(top_view_->GetCursor().IsEqual(cursor_hand));

  // Destruction of the child view should restore the parent frame's cursor.
  top_view_->GetCursorManager()->ViewBeingDestroyed(child_view);
  EXPECT_FALSE(top_view_->GetCursor().IsEqual(cursor_hand));

  delete child_view;
}

// Verify interactions between two independent OOPIFs, including interleaving
// cursor updates and mouse movements. This simulates potential race
// conditions between cursor updates.
TEST_F(CursorManagerTest, CursorOverMultipleChildViews) {
  MockRenderWidgetHostViewForCursors* child_view1 =
      new MockRenderWidgetHostViewForCursors(false);
  MockRenderWidgetHostViewForCursors* child_view2 =
      new MockRenderWidgetHostViewForCursors(false);

  CursorInfo cursor_info_hand(blink::WebCursorInfo::kTypeHand);
  WebCursor cursor_hand;
  cursor_hand.InitFromCursorInfo(cursor_info_hand);

  CursorInfo cursor_info_cross(blink::WebCursorInfo::kTypeCross);
  WebCursor cursor_cross;
  cursor_cross.InitFromCursorInfo(cursor_info_cross);

  CursorInfo cursor_info_pointer(blink::WebCursorInfo::kTypePointer);
  WebCursor cursor_pointer;
  cursor_pointer.InitFromCursorInfo(cursor_info_pointer);

  // Initialize each View to a different cursor.
  top_view_->GetCursorManager()->UpdateCursor(top_view_, cursor_hand);
  top_view_->GetCursorManager()->UpdateCursor(child_view1, cursor_cross);
  top_view_->GetCursorManager()->UpdateCursor(child_view2, cursor_pointer);
  EXPECT_TRUE(top_view_->GetCursor().IsEqual(cursor_hand));

  // Simulate moving the mouse between child views and receiving cursor updates.
  top_view_->GetCursorManager()->UpdateViewUnderCursor(child_view1);
  EXPECT_TRUE(top_view_->GetCursor().IsEqual(cursor_cross));
  top_view_->GetCursorManager()->UpdateViewUnderCursor(child_view2);
  EXPECT_TRUE(top_view_->GetCursor().IsEqual(cursor_pointer));

  // Simulate cursor updates to both child views. An update to child_view1
  // should not change the current cursor because the mouse is over
  // child_view2.
  top_view_->GetCursorManager()->UpdateCursor(child_view1, cursor_hand);
  EXPECT_TRUE(top_view_->GetCursor().IsEqual(cursor_pointer));
  top_view_->GetCursorManager()->UpdateCursor(child_view2, cursor_cross);
  EXPECT_TRUE(top_view_->GetCursor().IsEqual(cursor_cross));

  // Similarly, destroying child_view1 should have no effect on the cursor,
  // but destroying child_view2 should change it.
  top_view_->GetCursorManager()->ViewBeingDestroyed(child_view1);
  EXPECT_TRUE(top_view_->GetCursor().IsEqual(cursor_cross));
  top_view_->GetCursorManager()->ViewBeingDestroyed(child_view2);
  EXPECT_TRUE(top_view_->GetCursor().IsEqual(cursor_hand));

  delete child_view1;
  delete child_view2;
}

}  // namespace content

#endif  // defined(USE_AURA) || defined(OS_MACOSX)