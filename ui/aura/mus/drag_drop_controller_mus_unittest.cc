// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/mus/drag_drop_controller_mus.h"

#include <memory>

#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/aura/mus/drag_drop_controller_host.h"
#include "ui/aura/test/aura_mus_test_base.h"
#include "ui/aura/test/mus/test_window_tree.h"
#include "ui/events/event_utils.h"

namespace aura {
namespace {

class DragDropControllerMusTest : public test::AuraMusWmTestBase {
 public:
  void SetUp() override {
    AuraMusWmTestBase::SetUp();
    drag_drop_controller_ = base::MakeUnique<DragDropControllerMus>(
        &drag_drop_controller_host_, window_tree());
    drag_drop_controller_->set_should_block_during_drag_drop(false);
  }

  void TearDown() override {
    drag_drop_controller_.reset();
    AuraMusWmTestBase::TearDown();
  }

 protected:
  std::unique_ptr<DragDropControllerMus> drag_drop_controller_;

 private:
  class TestDragDropControllerHost : public DragDropControllerHost {
   public:
    TestDragDropControllerHost() : serial_(0u) {}

    uint32_t CreateChangeIdForDrag(WindowMus* window) override {
      return serial_++;
    }

   private:
    uint32_t serial_;

  } drag_drop_controller_host_;
};

class TestEnvObserver : public aura::EnvObserver {
 public:
  TestEnvObserver() : started_event_data(nullptr), ended_event_data(nullptr) {}

  void OnWindowInitialized(aura::Window* window) override {}

  void OnDragStarted(const ui::OSExchangeData* data) override {
    started_event_data = data;
  }

  void OnDragEnded(const ui::OSExchangeData* data) override {
    ended_event_data = data;
  }

  const ui::OSExchangeData* started_event_data;
  const ui::OSExchangeData* ended_event_data;
};

TEST_F(DragDropControllerMusTest, DragStartedAndEndedEvents) {
  TestEnvObserver observer;
  aura::Env* instance = aura::Env::GetInstance();
  ASSERT_TRUE(instance);

  instance->AddObserver(&observer);

  ui::OSExchangeData data;
  data.SetString(base::UTF8ToUTF16("I am being dragged"));
  {
    std::unique_ptr<aura::Window> window(
        CreateNormalWindow(0, root_window(), nullptr));
    drag_drop_controller_->StartDragAndDrop(
        data, window->GetRootWindow(), window.get(), gfx::Point(5, 5),
        ui::DragDropTypes::DRAG_MOVE,
        ui::DragDropTypes::DRAG_EVENT_SOURCE_MOUSE);

    ui::MouseEvent e(ui::ET_MOUSE_DRAGGED, gfx::Point(200, 0),
                     gfx::Point(200, 0), ui::EventTimeForNow(), ui::EF_NONE,
                     ui::EF_NONE);

    EXPECT_EQ(&data, observer.started_event_data);
    EXPECT_EQ(&data, observer.ended_event_data);
  }

  instance->RemoveObserver(&observer);
}

}  // namespace
}  // namespace aura
