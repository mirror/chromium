// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "components/exo/buffer.h"
#include "components/exo/notification_surface.h"
#include "components/exo/surface.h"
#include "components/exo/test/exo_test_helper.h"
#include "ui/arc/notification/arc_notification_content_view.h"
#include "ui/arc/notification/arc_notification_delegate.h"
#include "ui/arc/notification/arc_notification_item.h"
#include "ui/arc/notification/arc_notification_surface.h"
#include "ui/arc/notification/arc_notification_surface_manager_impl.h"
#include "ui/arc/notification/arc_notification_view.h"
#include "ui/aura/test/test_window_delegate.h"
#include "ui/aura/window.h"
#include "ui/message_center/notification.h"
#include "ui/message_center/views/message_center_controller.h"
#include "ui/message_center/views/message_view_factory.h"
#include "ui/message_center/views/notification_control_buttons_view.h"
#include "ui/message_center/views/padded_button.h"
#include "ui/views/test/views_test_base.h"

namespace arc {

namespace {

constexpr char kNotificationIdPrefix[] = "ARC_NOTIFICATION_";

}  // anonymous namespace

class MockArcNotificationItem : public ArcNotificationItem {
 public:
  MockArcNotificationItem(const std::string& notification_key)
      : notification_key_(notification_key),
        notification_id_(kNotificationIdPrefix + notification_key),
        weak_factory_(this) {}

  // Methods for testing.
  size_t count_close() { return count_close_; }
  base::WeakPtr<MockArcNotificationItem> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

  // Overriding methods for testing.
  void Close(bool by_user) override { count_close_++; }
  const gfx::ImageSkia& GetSnapshot() const override { return snapshot_; }
  const std::string& GetNotificationKey() const override {
    return notification_key_;
  }
  const std::string& GetNotificationId() const override {
    return notification_id_;
  }

  // Overriding methods for returning dummy data or doing nothing.
  void OnClosedFromAndroid() override {}
  void Click() override {}
  void ToggleExpansion() override {}
  void OpenSettings() override {}
  void AddObserver(Observer* observer) override {}
  void RemoveObserver(Observer* observer) override {}
  void IncrementWindowRefCount() override {}
  void DecrementWindowRefCount() override {}
  bool GetPinned() const override { return false; }
  bool IsOpeningSettingsSupported() const override { return true; }
  mojom::ArcNotificationExpandState GetExpandState() const override {
    return mojom::ArcNotificationExpandState::FIXED_SIZE;
  }
  mojom::ArcNotificationShownContents GetShownContents() const override {
    return mojom::ArcNotificationShownContents::CONTENTS_SHOWN;
  }
  const base::string16& GetAccessibleName() const override {
    return base::EmptyString16();
  };
  void OnUpdatedFromAndroid(mojom::ArcNotificationDataPtr data) override {}

 private:
  std::string notification_key_;
  std::string notification_id_;
  gfx::ImageSkia snapshot_;
  size_t count_close_ = 0;

  base::WeakPtrFactory<MockArcNotificationItem> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MockArcNotificationItem);
};

class TestMessageCenterController
    : public message_center::MessageCenterController {
 public:
  TestMessageCenterController() = default;

  // MessageCenterController
  void ClickOnNotification(const std::string& notification_id) override {
    // For this test, this method should not be invoked.
    NOTREACHED();
  }

  void RemoveNotification(const std::string& notification_id,
                          bool by_user) override {
    removed_ids_.insert(notification_id);
  }

  std::unique_ptr<ui::MenuModel> CreateMenuModel(
      const message_center::NotifierId& notifier_id,
      const base::string16& display_source) override {
    // For this test, this method should not be invoked.
    NOTREACHED();
    return nullptr;
  }

  bool HasClickedListener(const std::string& notification_id) override {
    return false;
  }

  void ClickOnNotificationButton(const std::string& notification_id,
                                 int button_index) override {
    // For this test, this method should not be invoked.
    NOTREACHED();
  }

  void ClickOnSettingsButton(const std::string& notification_id) override {
    // For this test, this method should not be invoked.
    NOTREACHED();
  }

  void UpdateNotificationSize(const std::string& notification_id) override {}

  bool IsRemoved(const std::string& notification_id) const {
    return (removed_ids_.find(notification_id) != removed_ids_.end());
  }

 private:
  std::set<std::string> removed_ids_;

  DISALLOW_COPY_AND_ASSIGN(TestMessageCenterController);
};

class DummyEvent : public ui::Event {
 public:
  DummyEvent() : Event(ui::ET_UNKNOWN, base::TimeTicks(), 0) {}
  ~DummyEvent() override = default;
};

class ArcNotificationContentViewTest : public views::ViewsTestBase {
 public:
  ArcNotificationContentViewTest() = default;
  ~ArcNotificationContentViewTest() override = default;

  void SetUp() override {
    views::ViewsTestBase::SetUp();

    surface_manager_ = base::MakeUnique<ArcNotificationSurfaceManagerImpl>();
  }

  void TearDown() override {
    // Widget and view need to be closed before TearDown() if have been created.
    EXPECT_FALSE(wrapper_widget_);
    EXPECT_FALSE(notification_view_);

    notification_surface_.reset();
    surface_.reset();

    surface_manager_.reset();

    views::ViewsTestBase::TearDown();
  }

  void PressCloseButton() {
    DummyEvent dummy_event;
    auto* control_buttons_view =
        GetArcNotificationContentView()->control_buttons_view_;
    message_center::PaddedButton* close_button =
        control_buttons_view->close_button();
    ASSERT_NE(nullptr, close_button);
    control_buttons_view->ButtonPressed(close_button, dummy_event);
  }

  void CreateAndShowNotificationView(
      const message_center::Notification& notification) {
    DCHECK(!notification_view_);

    notification_view_.reset(static_cast<ArcNotificationView*>(
        message_center::MessageViewFactory::Create(controller(), notification,
                                                   true)));
    notification_view_->set_owned_by_client();
    views::Widget::InitParams params(
        CreateParams(views::Widget::InitParams::TYPE_POPUP));

    params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
    wrapper_widget_ = base::MakeUnique<views::Widget>();
    wrapper_widget_->Init(params);
    wrapper_widget_->SetContentsView(notification_view_.get());
    wrapper_widget_->SetSize(notification_view_->GetPreferredSize());
  }

  void CloseNotificationView() {
    wrapper_widget_->Close();
    wrapper_widget_.reset();

    notification_view_.reset();
  }

  void PrepareSurface(std::string& notification_key) {
    exo::test::ExoTestHelper exo_test_helper;

    surface_ = base::MakeUnique<exo::Surface>();
    notification_surface_ = base::MakeUnique<exo::NotificationSurface>(
        surface_manager(), surface_.get(), notification_key);

    const gfx::Rect bounds(100, 100, 300, 300);
    exo::Buffer buffer(exo_test_helper.CreateGpuMemoryBuffer(bounds.size()));
    surface_->Attach(&buffer);

    surface_->Commit();
  }

  message_center::Notification CreateNotification(
      MockArcNotificationItem* notification_item) {
    return message_center::Notification(
        message_center::NOTIFICATION_TYPE_CUSTOM,
        notification_item->GetNotificationId(), base::UTF8ToUTF16("title"),
        base::UTF8ToUTF16("message"), gfx::Image(), base::UTF8ToUTF16("arc"),
        GURL(),
        message_center::NotifierId(message_center::NotifierId::SYSTEM_COMPONENT,
                                   "ARC_NOTIFICATION"),
        message_center::RichNotificationData(),
        new ArcNotificationDelegate(notification_item->GetWeakPtr()));
  }

  TestMessageCenterController* controller() { return &controller_; }
  ArcNotificationSurfaceManagerImpl* surface_manager() const {
    return surface_manager_.get();
  }
  views::Widget* widget() { return notification_view_->GetWidget(); }

  ArcNotificationContentView* GetArcNotificationContentView() {
    views::View* view = notification_view_->contents_view_;
    EXPECT_EQ(ArcNotificationContentView::kViewClassName, view->GetClassName());
    return static_cast<ArcNotificationContentView*>(view);
  }

 private:
  TestMessageCenterController controller_;
  std::unique_ptr<ArcNotificationSurfaceManagerImpl> surface_manager_;
  std::unique_ptr<ArcNotificationView> notification_view_;
  std::unique_ptr<views::Widget> wrapper_widget_;
  std::unique_ptr<exo::Surface> surface_;
  std::unique_ptr<exo::NotificationSurface> notification_surface_;

  DISALLOW_COPY_AND_ASSIGN(ArcNotificationContentViewTest);
};

TEST_F(ArcNotificationContentViewTest, CreateSurfaceAfterNotification) {
  std::string notification_key("notification id");

  auto notification_item =
      base::MakeUnique<MockArcNotificationItem>(notification_key);
  message_center::Notification notification =
      CreateNotification(notification_item.get());

  PrepareSurface(notification_key);

  CreateAndShowNotificationView(notification);
  CloseNotificationView();
}

TEST_F(ArcNotificationContentViewTest, CreateSurfaceBeforeNotification) {
  std::string notification_key("notification id");

  PrepareSurface(notification_key);

  auto notification_item =
      base::MakeUnique<MockArcNotificationItem>(notification_key);
  message_center::Notification notification =
      CreateNotification(notification_item.get());

  CreateAndShowNotificationView(notification);
  CloseNotificationView();
}

TEST_F(ArcNotificationContentViewTest, CreateNotificationWithoutSurface) {
  std::string notification_key("notification id");

  auto notification_item =
      base::MakeUnique<MockArcNotificationItem>(notification_key);
  message_center::Notification notification =
      CreateNotification(notification_item.get());

  CreateAndShowNotificationView(notification);
  CloseNotificationView();
}

TEST_F(ArcNotificationContentViewTest, CloseButton) {
  std::string notification_key("notification id");

  auto notification_item =
      base::MakeUnique<MockArcNotificationItem>(notification_key);
  PrepareSurface(notification_key);
  message_center::Notification notification =
      CreateNotification(notification_item.get());
  CreateAndShowNotificationView(notification);

  // EXPECT_EQ(1u, surface_manager()->surface_found_count());
  EXPECT_FALSE(controller()->IsRemoved(notification_item->GetNotificationId()));
  PressCloseButton();
  EXPECT_TRUE(controller()->IsRemoved(notification_item->GetNotificationId()));

  CloseNotificationView();
}

TEST_F(ArcNotificationContentViewTest, ReuseSurfaceAfterClosing) {
  std::string notification_key("notification id");

  auto notification_item =
      base::MakeUnique<MockArcNotificationItem>(notification_key);
  message_center::Notification notification =
      CreateNotification(notification_item.get());

  PrepareSurface(notification_key);

  // Use the created surface.
  CreateAndShowNotificationView(notification);
  CloseNotificationView();

  // Reuse.
  CreateAndShowNotificationView(notification);
  CloseNotificationView();

  // Reuse again.
  CreateAndShowNotificationView(notification);
  CloseNotificationView();
}

}  // namespace arc
