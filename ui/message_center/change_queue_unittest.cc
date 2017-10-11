// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/message_center/change_queue.h"

#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/message_center/message_center_impl.h"

using base::UTF8ToUTF16;

namespace message_center {

namespace {

Notification* CreateSimpleNotification(const std::string& id) {
  RichNotificationData optional_fields;
  optional_fields.buttons.push_back(ButtonInfo(UTF8ToUTF16("foo")));
  optional_fields.buttons.push_back(ButtonInfo(UTF8ToUTF16("foo")));
  return new Notification(
      NOTIFICATION_TYPE_SIMPLE, id, UTF8ToUTF16("title"), UTF8ToUTF16(id),
      gfx::Image() /* icon */, base::string16() /* display_source */, GURL(),
      NotifierId(NotifierId::APPLICATION, "change_queue_test"), optional_fields,
      NULL);
}

}  // namespace

class ChangeQueueTest : public testing::Test {
 public:
  ChangeQueueTest() = default;
  ~ChangeQueueTest() override = default;

  MessageCenterImpl* message_center() { return &message_center_; }

 private:
  MessageCenterImpl message_center_;
};

TEST_F(ChangeQueueTest, Queueing) {
  ChangeQueue queue;
  std::string id("id1");
  std::string id2("id2");
  NotifierId notifier_id1(NotifierId::APPLICATION, "app1");

  // First, add and update a notification to ensure updates happen
  // normally.
  std::unique_ptr<Notification> notification(CreateSimpleNotification(id));
  message_center()->AddNotification(std::move(notification));
  notification.reset(CreateSimpleNotification(id2));
  message_center()->UpdateNotification(id, std::move(notification));
  EXPECT_TRUE(message_center()->FindVisibleNotificationById(id2));
  EXPECT_FALSE(message_center()->FindVisibleNotificationById(id));

  // Then update a notification; nothing should have happened.
  notification.reset(CreateSimpleNotification(id));
  queue.UpdateNotification(id2, std::move(notification));
  EXPECT_TRUE(message_center()->FindVisibleNotificationById(id2));
  EXPECT_FALSE(message_center()->FindVisibleNotificationById(id));

  // Apply the changes from the queue.
  queue.ApplyChanges(message_center());

  // Close the message center; then the update should have propagated.
  EXPECT_FALSE(message_center()->FindVisibleNotificationById(id2));
  EXPECT_TRUE(message_center()->FindVisibleNotificationById(id));
}

TEST_F(ChangeQueueTest, SimpleQueueing) {
  ChangeQueue queue;
  std::string ids[6] = {"0", "1", "2", "3", "4", "5"};
  NotifierId notifier_id1(NotifierId::APPLICATION, "app1");

  std::unique_ptr<Notification> notification;
  // Prepare initial notifications on NotificationList.
  // NL: ["0", "1", "2", "4"]
  int i = 0;
  for (; i < 3; i++) {
    notification.reset(CreateSimpleNotification(ids[i]));
    message_center()->AddNotification(std::move(notification));
  }
  for (i = 0; i < 3; i++) {
    EXPECT_TRUE(message_center()->FindVisibleNotificationById(ids[i]));
  }
  for (; i < 6; i++) {
    EXPECT_FALSE(message_center()->FindVisibleNotificationById(ids[i]));
  }

  // This should update notification "1" to have id "4".
  notification.reset(CreateSimpleNotification(ids[4]));
  queue.AddNotification(std::move(notification));

  // Change the ID: "2" -> "5", "5" -> "3".
  notification.reset(CreateSimpleNotification(ids[5]));
  queue.UpdateNotification(ids[2], std::move(notification));
  notification.reset(CreateSimpleNotification(ids[3]));
  queue.UpdateNotification(ids[5], std::move(notification));

  // This should update notification "4" to "5".
  notification.reset(CreateSimpleNotification(ids[5]));
  queue.UpdateNotification(ids[4], std::move(notification));

  // This should create a new "4", that doesn't overwrite the update from 4
  // before.
  notification.reset(CreateSimpleNotification(ids[4]));
  queue.AddNotification(std::move(notification));

  // The NL should still be the same: ["0", "1", "2", "4"]
  EXPECT_TRUE(message_center()->FindVisibleNotificationById(ids[0]));
  EXPECT_TRUE(message_center()->FindVisibleNotificationById(ids[1]));
  EXPECT_TRUE(message_center()->FindVisibleNotificationById(ids[2]));
  EXPECT_FALSE(message_center()->FindVisibleNotificationById(ids[3]));
  EXPECT_FALSE(message_center()->FindVisibleNotificationById(ids[4]));
  EXPECT_FALSE(message_center()->FindVisibleNotificationById(ids[5]));
  EXPECT_EQ(message_center()->GetVisibleNotifications().size(), 3u);

  // Apply the changes from the queue.
  queue.ApplyChanges(message_center());

  // Changes should be applied.
  EXPECT_TRUE(message_center()->FindVisibleNotificationById(ids[0]));
  EXPECT_TRUE(message_center()->FindVisibleNotificationById(ids[1]));
  EXPECT_FALSE(message_center()->FindVisibleNotificationById(ids[2]));
  EXPECT_TRUE(message_center()->FindVisibleNotificationById(ids[3]));
  EXPECT_TRUE(message_center()->FindVisibleNotificationById(ids[4]));
  EXPECT_TRUE(message_center()->FindVisibleNotificationById(ids[5]));
  EXPECT_EQ(message_center()->GetVisibleNotifications().size(), 5u);
}

TEST_F(ChangeQueueTest, ComplexQueueing) {
  ChangeQueue queue;
  std::string ids[11] = {"0", "1", "2", "3", "4", "5",
                         "6", "7", "8", "9", "10"};
  NotifierId notifier_id1(NotifierId::APPLICATION, "app1");

  std::unique_ptr<Notification> notification;
  // Add some notifications
  int i = 0;
  for (; i < 6; i++) {
    notification.reset(CreateSimpleNotification(ids[i]));
    notification->set_title(base::ASCIIToUTF16("ORIGINAL TITLE"));
    message_center()->AddNotification(std::move(notification));
  }
  for (i = 0; i < 6; i++) {
    EXPECT_TRUE(message_center()->FindVisibleNotificationById(ids[i]));
  }
  for (; i < 11; i++) {
    EXPECT_FALSE(message_center()->FindVisibleNotificationById(ids[i]));
  }

  notification.reset(CreateSimpleNotification(ids[10]));
  message_center()->AddNotification(std::move(notification));

  // Notification 0: (Exists) -> Removed
  queue.RemoveNotification(ids[0], false);

  // Notification 1: (Exists) -> Removed (by user)
  queue.RemoveNotification(ids[1], true);

  // Notification 2: (Exists) -> Removed -> Added
  queue.RemoveNotification(ids[2], false);
  notification.reset(CreateSimpleNotification(ids[2]));
  notification->set_title(base::ASCIIToUTF16("NEW TITLE 2"));
  queue.UpdateNotification(ids[2], std::move(notification));

  // Notification 3: (Exists) -> Removed -> Updated
  queue.RemoveNotification(ids[3], false);
  notification.reset(CreateSimpleNotification(ids[3]));
  notification->set_title(base::ASCIIToUTF16("NEW TITLE 3"));
  queue.UpdateNotification(ids[3], std::move(notification));

  // Notification 4: (Exists) -> Removed -> Added -> Removed
  queue.RemoveNotification(ids[4], false);
  notification.reset(CreateSimpleNotification(ids[4]));
  queue.AddNotification(std::move(notification));
  queue.RemoveNotification(ids[4], false);

  // Notification 5: (Exists) -> Updated
  notification.reset(CreateSimpleNotification(ids[5]));
  notification->set_title(base::ASCIIToUTF16("NEW TITLE 5"));
  queue.UpdateNotification(ids[5], std::move(notification));

  // Notification 6: Created -> Updated
  notification.reset(CreateSimpleNotification(ids[6]));
  queue.UpdateNotification(ids[6], std::move(notification));

  // Notification 7: Created -> Updated -> Removed
  notification.reset(CreateSimpleNotification(ids[7]));
  queue.UpdateNotification(ids[7], std::move(notification));
  queue.RemoveNotification(ids[7], false);

  // Notification 8: Created -> Added -> Updated
  notification.reset(CreateSimpleNotification(ids[8]));
  notification->set_title(base::ASCIIToUTF16("UPDATING 8-1"));
  queue.AddNotification(std::move(notification));
  notification.reset(CreateSimpleNotification(ids[8]));
  notification->set_title(base::ASCIIToUTF16("NEW TITLE 8"));
  queue.UpdateNotification(ids[8], std::move(notification));

  // Notification 9: Created -> Added -> Updated -> Removed
  notification.reset(CreateSimpleNotification(ids[9]));
  queue.AddNotification(std::move(notification));
  notification.reset(CreateSimpleNotification(ids[9]));
  queue.UpdateNotification(ids[9], std::move(notification));
  queue.RemoveNotification(ids[9], false);

  // Notification 10: (exists)-> Updated -> Removed -> Added -> Updated
  // Step 1) Progress is updated immediately before removed.
  notification.reset(CreateSimpleNotification(ids[10]));
  queue.UpdateNotification(ids[10], std::move(notification));
  queue.RemoveNotification(ids[10], false);
  // Step 2) Progress isn't updated after removed.
  notification.reset(CreateSimpleNotification(ids[10]));
  queue.AddNotification(std::move(notification));
  notification.reset(CreateSimpleNotification(ids[10]));
  notification->set_title(base::ASCIIToUTF16("NEW TITLE 10"));
  queue.UpdateNotification(ids[10], std::move(notification));

  // All changes except the notification 1 and 10 are not refrected.
  EXPECT_TRUE(message_center()->FindVisibleNotificationById(ids[0]));
  EXPECT_EQ(base::ASCIIToUTF16("ORIGINAL TITLE"),
            message_center()->FindVisibleNotificationById(ids[0])->title());
  EXPECT_TRUE(message_center()->FindVisibleNotificationById(ids[1]));
  EXPECT_TRUE(message_center()->FindVisibleNotificationById(ids[2]));
  EXPECT_EQ(base::ASCIIToUTF16("ORIGINAL TITLE"),
            message_center()->FindVisibleNotificationById(ids[2])->title());
  EXPECT_TRUE(message_center()->FindVisibleNotificationById(ids[3]));
  EXPECT_EQ(base::ASCIIToUTF16("ORIGINAL TITLE"),
            message_center()->FindVisibleNotificationById(ids[3])->title());
  EXPECT_TRUE(message_center()->FindVisibleNotificationById(ids[4]));
  EXPECT_EQ(base::ASCIIToUTF16("ORIGINAL TITLE"),
            message_center()->FindVisibleNotificationById(ids[4])->title());
  EXPECT_TRUE(message_center()->FindVisibleNotificationById(ids[5]));
  EXPECT_EQ(base::ASCIIToUTF16("ORIGINAL TITLE"),
            message_center()->FindVisibleNotificationById(ids[5])->title());
  EXPECT_FALSE(message_center()->FindVisibleNotificationById(ids[6]));
  EXPECT_FALSE(message_center()->FindVisibleNotificationById(ids[7]));
  EXPECT_FALSE(message_center()->FindVisibleNotificationById(ids[8]));
  EXPECT_FALSE(message_center()->FindVisibleNotificationById(ids[9]));
  EXPECT_TRUE(message_center()->FindVisibleNotificationById(ids[10]));
  EXPECT_EQ(message_center()->GetVisibleNotifications().size(), 7u);

  queue.ApplyChanges(message_center());

  EXPECT_FALSE(message_center()->FindVisibleNotificationById(ids[0]));
  EXPECT_FALSE(message_center()->FindVisibleNotificationById(ids[1]));
  EXPECT_TRUE(message_center()->FindVisibleNotificationById(ids[2]));
  EXPECT_EQ(base::ASCIIToUTF16("NEW TITLE 2"),
            message_center()->FindVisibleNotificationById(ids[2])->title());
  EXPECT_TRUE(message_center()->FindVisibleNotificationById(ids[3]));
  EXPECT_EQ(base::ASCIIToUTF16("NEW TITLE 3"),
            message_center()->FindVisibleNotificationById(ids[3])->title());
  EXPECT_FALSE(message_center()->FindVisibleNotificationById(ids[4]));
  EXPECT_TRUE(message_center()->FindVisibleNotificationById(ids[5]));
  EXPECT_EQ(base::ASCIIToUTF16("NEW TITLE 5"),
            message_center()->FindVisibleNotificationById(ids[5])->title());
  EXPECT_FALSE(message_center()->FindVisibleNotificationById(ids[6]));
  EXPECT_FALSE(message_center()->FindVisibleNotificationById(ids[7]));
  EXPECT_TRUE(message_center()->FindVisibleNotificationById(ids[8]));
  EXPECT_EQ(base::ASCIIToUTF16("NEW TITLE 8"),
            message_center()->FindVisibleNotificationById(ids[8])->title());
  EXPECT_FALSE(message_center()->FindVisibleNotificationById(ids[9]));
  EXPECT_TRUE(message_center()->FindVisibleNotificationById(ids[10]));
  EXPECT_EQ(base::ASCIIToUTF16("NEW TITLE 10"),
            message_center()->FindVisibleNotificationById(ids[10])->title());
  EXPECT_EQ(message_center()->GetVisibleNotifications().size(), 5u);
}

}  // namespace message_center
