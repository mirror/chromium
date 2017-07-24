// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/mus/drag_drop_controller_mus.h"

#include <memory>

#include "base/callback_forward.h"
#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "ui/aura/client/drag_drop_client_observer.h"
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

class TestObserver : public client::DragDropClientObserver {
 public:
  TestObserver(DragDropControllerMus* controller)
      : controller_(controller),
        started_event_data_(nullptr),
        ended_event_data_(nullptr) {
    controller_->AddObserver(this);
  }

  ~TestObserver() override { controller_->RemoveObserver(this); }

  // Overrides from client::DragDropClientObserver:
  void OnDragStarted(const ui::OSExchangeData* data) override {
    EXPECT_EQ(nullptr, started_event_data_);
    EXPECT_EQ(nullptr, ended_event_data_);
    started_event_data_ = data;
  }

  void OnDragEnded(const ui::OSExchangeData* data) override {
    EXPECT_EQ(&data_, started_event_data_);
    EXPECT_EQ(nullptr, ended_event_data_);
    ended_event_data_ = data;
  }

  void OnInternalLoopRun() {
    EXPECT_EQ(&data_, started_event_data_);
    EXPECT_EQ(nullptr, ended_event_data_);
    controller_->OnPerformDragDropCompleted(0);
  }

  ui::OSExchangeData* data() { return &data_; }
  const ui::OSExchangeData* started_event_data() { return started_event_data_; }
  const ui::OSExchangeData* ended_event_data() { return ended_event_data_; }

 private:
  DragDropControllerMus* controller_;
  ui::OSExchangeData data_;
  const ui::OSExchangeData* started_event_data_;
  const ui::OSExchangeData* ended_event_data_;
};

TEST_F(DragDropControllerMusTest, DragStartedAndEndedEvents) {
  TestObserver observer(drag_drop_controller_.get());
  observer.data()->SetString(base::UTF8ToUTF16("I am being dragged"));
  std::unique_ptr<aura::Window> window(
      CreateNormalWindow(0, root_window(), nullptr));

  // Posted task will be run when the inner loop runs in StartDragAndDrop.
  base::PostTask(FROM_HERE, base::BindOnce(&TestObserver::OnRunInternalLoop,
                                           base::Unretained(&observer)));

  drag_drop_controller_->StartDragAndDrop(
      *observer.data(), window->GetRootWindow(), window.get(), gfx::Point(5, 5),
      ui::DragDropTypes::DRAG_MOVE, ui::DragDropTypes::DRAG_EVENT_SOURCE_MOUSE);

  EXPECT_EQ(observer.data(), observer.started_event_data());
  EXPECT_EQ(observer.data(), observer.ended_event_data());
}

}  // namespace
}  // namespace aura
