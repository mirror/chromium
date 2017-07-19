// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/mus/window_tree_client.h"

#include "components/viz/client/hit_test_data_provider.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/mus/window_mus.h"
#include "ui/aura/mus/window_port_mus.h"
#include "ui/aura/test/aura_mus_test_base.h"
#include "ui/aura/window.h"
#include "ui/aura/window_targeter.h"
#include "ui/gfx/geometry/rect.h"

namespace aura {

namespace {

const int kMouseInset = -5;
const int kTouchInset = -10;

Id server_id(Window* window) {
  return WindowMus::Get(window)->server_id();
}

}  // namespace

// Creates a root window and child windows. Maintains a cc:LayerTreeFrameSink
// to help exercise its viz::HitTestDataProvider.
class HitTestDataProviderMusTest : public test::AuraTestBaseMus {
 public:
  HitTestDataProviderMusTest() {}
  ~HitTestDataProviderMusTest() override {}

  void SetUp() override {
    test::AuraTestBaseMus::SetUp();

    root_ = base::MakeUnique<Window>(nullptr);
    root_->Init(ui::LAYER_NOT_DRAWN);
    root_->SetEventTargeter(base::MakeUnique<WindowTargeter>());
    root_->SetBounds(gfx::Rect(0, 0, 300, 200));

    window2_ = new Window(nullptr);
    window2_->Init(ui::LAYER_TEXTURED);
    window2_->SetBounds(gfx::Rect(20, 30, 40, 60));

    window3_ = new Window(nullptr);
    window3_->Init(ui::LAYER_TEXTURED);
    window3_->SetEventTargeter(base::MakeUnique<WindowTargeter>());
    window3_->SetBounds(gfx::Rect(50, 60, 100, 40));

    window4_ = new Window(nullptr);
    window4_->Init(ui::LAYER_TEXTURED);
    window4_->SetBounds(gfx::Rect(25, 10, 50, 20));

    window3_->AddChild(window4_);
    root_->AddChild(window2_);
    root_->AddChild(window3_);

    WindowPortMus* port = WindowPortMus::Get(root_.get());
    sink_ = port->CreateLayerTreeFrameSink();
  }

 protected:
  viz::HitTestDataProvider* hit_test_provider() {
    WindowPortMus* port = WindowPortMus::Get(root_.get());
    return port->local_layer_tree_frame_sink_->hit_test_data_provider_.get();
  }

  Window* root() { return root_.get(); }
  Window* window2() { return window2_; }
  Window* window3() { return window3_; }
  Window* window4() { return window4_; }

 private:
  std::unique_ptr<cc::LayerTreeFrameSink> sink_;
  std::unique_ptr<Window> root_;
  Window* window2_;
  Window* window3_;
  Window* window4_;

  DISALLOW_COPY_AND_ASSIGN(HitTestDataProviderMusTest);
};

// Custom WindowTargeter that expands hit-test regions of child windows.
class TestWindowTargeter : public WindowTargeter {
 public:
  TestWindowTargeter() {}
  ~TestWindowTargeter() override {}

 protected:
  // WindowTargeter:
  bool GetHitTestRects(aura::Window* window,
                       gfx::Rect* rect_mouse,
                       gfx::Rect* rect_touch) const override {
    if (rect_mouse) {
      *rect_mouse = gfx::Rect(window->bounds());
      rect_mouse->Inset(gfx::Insets(kMouseInset));
    }
    if (rect_touch) {
      *rect_touch = gfx::Rect(window->bounds());
      rect_touch->Inset(gfx::Insets(kTouchInset));
    }
    return true;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TestWindowTargeter);
};

// Tests that the order of reported hit-test regions matches windows Z-order.
TEST_F(HitTestDataProviderMusTest, Stacking) {
  std::unique_ptr<viz::HitTestDataProvider::HitTestRegionList> hit_test_data =
      hit_test_provider()->GetHitTestData();

  Window* expected_order_1[] = {window3(), window4(), window2()};
  EXPECT_EQ(arraysize(expected_order_1), hit_test_data->size());
  int i = 0;
  for (const auto& region : *hit_test_data) {
    EXPECT_EQ(0x03U, region.flags);
    EXPECT_EQ(WindowPortMus::Get(expected_order_1[i])->frame_sink_id(),
              region.frame_sink_id);
    EXPECT_EQ(server_id(expected_order_1[i]), region.window_id);
    EXPECT_EQ(expected_order_1[i]->bounds().ToString(), region.rect.ToString());
    i++;
  }

  root()->StackChildAbove(window2(), window3());
  hit_test_data = hit_test_provider()->GetHitTestData();

  Window* expected_order_2[] = {window2(), window3(), window4()};
  EXPECT_EQ(arraysize(expected_order_2), hit_test_data->size());
  i = 0;
  for (const auto& region : *hit_test_data) {
    EXPECT_EQ(0x03U, region.flags);
    EXPECT_EQ(WindowPortMus::Get(expected_order_2[i])->frame_sink_id(),
              region.frame_sink_id);
    EXPECT_EQ(server_id(expected_order_2[i]), region.window_id);
    EXPECT_EQ(expected_order_2[i]->bounds().ToString(), region.rect.ToString());
    i++;
  }
}

// Tests that the hit-test regions get expanded with a custom event targeter.
TEST_F(HitTestDataProviderMusTest, CustomTargeter) {
  window3()->SetEventTargeter(base::MakeUnique<TestWindowTargeter>());
  std::unique_ptr<viz::HitTestDataProvider::HitTestRegionList> hit_test_data =
      hit_test_provider()->GetHitTestData();

  // Children of a container that has the custom targeter installed will get
  // reported twice, once with hit-test bounds optimized for mouse events and
  // another time with bounds expanded more for touch input.
  Window* expected_windows[] = {window3(), window4(), window4(), window2()};
  uint32_t expected_flags[] = {0x03U, 0x01U, 0x02U, 0x03U};
  int expected_insets[] = {0, kMouseInset, kTouchInset, 0};
  ASSERT_EQ(arraysize(expected_windows), hit_test_data->size());
  ASSERT_EQ(arraysize(expected_flags), hit_test_data->size());
  ASSERT_EQ(arraysize(expected_insets), hit_test_data->size());
  int i = 0;
  for (const auto& region : *hit_test_data) {
    EXPECT_EQ(WindowPortMus::Get(expected_windows[i])->frame_sink_id(),
              region.frame_sink_id);
    EXPECT_EQ(server_id(expected_windows[i]), region.window_id);
    EXPECT_EQ(expected_flags[i], region.flags);
    gfx::Rect expected_bounds = expected_windows[i]->bounds();
    expected_bounds.Inset(gfx::Insets(expected_insets[i]));
    EXPECT_EQ(expected_bounds.ToString(), region.rect.ToString());
    i++;
  }
}

}  // namespace aura
