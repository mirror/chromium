// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/notifications/notification_template_builder.h"

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string16.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/notifications/notification_image_retainer.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/message_center/notification.h"

using message_center::Notification;
using message_center::NotifierId;
using message_center::RichNotificationData;

namespace {

const char kNotificationId[] = "notification_id";
const char kNotificationTitle[] = "My Title";
const char kNotificationMessage[] = "My Message";
const char kNotificationOrigin[] = "https://example.com";
const char kProfileId[] = "Default";

class MockNotificationImageRetainer : public NotificationImageRetainer {
 public:
  MockNotificationImageRetainer() : NotificationImageRetainer(nullptr) {}

  base::FilePath RegisterTemporaryImage(const gfx::Image& image,
                                        const std::string& profile_id,
                                        const GURL& origin) override {
    base::string16 file = base::string16(L"c:\\temp\\img") +
                          base::IntToString16(counter_++) +
                          base::string16(L".tmp");
    return base::FilePath(file);
  }

 private:
  int counter_ = 0;

  DISALLOW_COPY_AND_ASSIGN(MockNotificationImageRetainer);
};

bool FixedTime(base::Time* time) {
  base::Time::Exploded exploded = {0};
  exploded.year = 1998;
  exploded.month = 9;
  exploded.day_of_month = 4;
  exploded.hour = 1;
  exploded.minute = 2;
  exploded.second = 3;
  return base::Time::FromUTCExploded(exploded, time);
}

}  // namespace

class NotificationTemplateBuilderTest : public ::testing::Test {
 public:
  NotificationTemplateBuilderTest() = default;
  ~NotificationTemplateBuilderTest() override = default;

 protected:
  // Builds a notification object and initializes it to default values.
  std::unique_ptr<message_center::Notification> InitializeBasicNotification() {
    GURL origin_url(kNotificationOrigin);
    auto notification = std::make_unique<message_center::Notification>(
        message_center::NOTIFICATION_TYPE_SIMPLE, kNotificationId,
        base::UTF8ToUTF16(kNotificationTitle),
        base::UTF8ToUTF16(kNotificationMessage), gfx::Image() /* icon */,
        base::string16() /* display_source */, origin_url,
        NotifierId(origin_url), RichNotificationData(), nullptr /* delegate */);
    base::Time timestamp;
    if (!FixedTime(&timestamp))
      return nullptr;
    notification->set_timestamp(timestamp);
    return notification;
  }

  // Converts the notification data to XML and verifies it is as expected. Calls
  // must be wrapped in ASSERT_NO_FATAL_FAILURE().
  void VerifyXml(const message_center::Notification& notification,
                 const base::string16& xml_template) {
    MockNotificationImageRetainer image_retainer;
    template_ = NotificationTemplateBuilder::Build(
        kNotificationId, &image_retainer, kProfileId, notification);

    ASSERT_TRUE(template_);

    EXPECT_EQ(template_->GetNotificationTemplate(), xml_template);
  }

 protected:
  std::unique_ptr<NotificationTemplateBuilder> template_;

 private:
  DISALLOW_COPY_AND_ASSIGN(NotificationTemplateBuilderTest);
};

TEST_F(NotificationTemplateBuilderTest, SimpleToast) {
  std::unique_ptr<message_center::Notification> notification =
      InitializeBasicNotification();

  const wchar_t kExpectedXml[] =
      LR"(<toast launch="notification_id" displayTimestamp="1998-09-04T01:02:03Z">
 <visual>
  <binding template="ToastGeneric">
   <text>My Title</text>
   <text>My Message</text>
   <text placement="attribution">example.com</text>
  </binding>
 </visual>
</toast>
)";

  ASSERT_NO_FATAL_FAILURE(VerifyXml(*notification, kExpectedXml));
}

TEST_F(NotificationTemplateBuilderTest, Buttons) {
  std::unique_ptr<message_center::Notification> notification =
      InitializeBasicNotification();

  std::vector<message_center::ButtonInfo> buttons;
  buttons.emplace_back(base::ASCIIToUTF16("Button1"));
  buttons.emplace_back(base::ASCIIToUTF16("Button2"));
  notification->set_buttons(buttons);

  const wchar_t kExpectedXml[] =
      LR"(<toast launch="notification_id" displayTimestamp="1998-09-04T01:02:03Z">
 <visual>
  <binding template="ToastGeneric">
   <text>My Title</text>
   <text>My Message</text>
   <text placement="attribution">example.com</text>
  </binding>
 </visual>
 <actions>
  <action activationType="foreground" content="Button1" arguments="buttonIndex=0"/>
  <action activationType="foreground" content="Button2" arguments="buttonIndex=1"/>
 </actions>
</toast>
)";

  ASSERT_NO_FATAL_FAILURE(VerifyXml(*notification, kExpectedXml));
}

TEST_F(NotificationTemplateBuilderTest, InlineReplies) {
  std::unique_ptr<message_center::Notification> notification =
      InitializeBasicNotification();

  std::vector<message_center::ButtonInfo> buttons;
  message_center::ButtonInfo button1(base::ASCIIToUTF16("Button1"));
  button1.type = message_center::ButtonType::TEXT;
  button1.placeholder = base::ASCIIToUTF16("Reply here");
  buttons.emplace_back(button1);
  buttons.emplace_back(base::ASCIIToUTF16("Button2"));
  notification->set_buttons(buttons);

  const wchar_t kExpectedXml[] =
      LR"(<toast launch="notification_id" displayTimestamp="1998-09-04T01:02:03Z">
 <visual>
  <binding template="ToastGeneric">
   <text>My Title</text>
   <text>My Message</text>
   <text placement="attribution">example.com</text>
  </binding>
 </visual>
 <actions>
  <input id="userResponse" type="text" placeHolderContent="Reply here"/>
  <action activationType="foreground" content="Button1" arguments="buttonIndex=0"/>
  <action activationType="foreground" content="Button2" arguments="buttonIndex=1"/>
 </actions>
</toast>
)";

  ASSERT_NO_FATAL_FAILURE(VerifyXml(*notification, kExpectedXml));
}

TEST_F(NotificationTemplateBuilderTest, InlineRepliesDoubleInput) {
  std::unique_ptr<message_center::Notification> notification =
      InitializeBasicNotification();

  std::vector<message_center::ButtonInfo> buttons;
  message_center::ButtonInfo button1(base::ASCIIToUTF16("Button1"));
  button1.type = message_center::ButtonType::TEXT;
  button1.placeholder = base::ASCIIToUTF16("Reply here");
  buttons.emplace_back(button1);
  message_center::ButtonInfo button2(base::ASCIIToUTF16("Button2"));
  button2.type = message_center::ButtonType::TEXT;
  button2.placeholder = base::ASCIIToUTF16("Should not appear");
  buttons.emplace_back(button2);
  notification->set_buttons(buttons);

  const wchar_t kExpectedXml[] =
      LR"(<toast launch="notification_id" displayTimestamp="1998-09-04T01:02:03Z">
 <visual>
  <binding template="ToastGeneric">
   <text>My Title</text>
   <text>My Message</text>
   <text placement="attribution">example.com</text>
  </binding>
 </visual>
 <actions>
  <input id="userResponse" type="text" placeHolderContent="Reply here"/>
  <action activationType="foreground" content="Button1" arguments="buttonIndex=0"/>
  <action activationType="foreground" content="Button2" arguments="buttonIndex=1"/>
 </actions>
</toast>
)";

  ASSERT_NO_FATAL_FAILURE(VerifyXml(*notification, kExpectedXml));
}

TEST_F(NotificationTemplateBuilderTest, InlineRepliesTextTypeNotFirst) {
  std::unique_ptr<message_center::Notification> notification =
      InitializeBasicNotification();

  std::vector<message_center::ButtonInfo> buttons;
  buttons.emplace_back(base::ASCIIToUTF16("Button1"));
  message_center::ButtonInfo button2(base::ASCIIToUTF16("Button2"));
  button2.type = message_center::ButtonType::TEXT;
  button2.placeholder = base::ASCIIToUTF16("Reply here");
  buttons.emplace_back(button2);
  notification->set_buttons(buttons);

  const wchar_t kExpectedXml[] =
      LR"(<toast launch="notification_id" displayTimestamp="1998-09-04T01:02:03Z">
 <visual>
  <binding template="ToastGeneric">
   <text>My Title</text>
   <text>My Message</text>
   <text placement="attribution">example.com</text>
  </binding>
 </visual>
 <actions>
  <input id="userResponse" type="text" placeHolderContent="Reply here"/>
  <action activationType="foreground" content="Button1" arguments="buttonIndex=0"/>
  <action activationType="foreground" content="Button2" arguments="buttonIndex=1"/>
 </actions>
</toast>
)";

  ASSERT_NO_FATAL_FAILURE(VerifyXml(*notification, kExpectedXml));
}

TEST_F(NotificationTemplateBuilderTest, Silent) {
  std::unique_ptr<message_center::Notification> notification =
      InitializeBasicNotification();
  notification->set_silent(true);

  const wchar_t kExpectedXml[] =
      LR"(<toast launch="notification_id" displayTimestamp="1998-09-04T01:02:03Z">
 <visual>
  <binding template="ToastGeneric">
   <text>My Title</text>
   <text>My Message</text>
   <text placement="attribution">example.com</text>
  </binding>
 </visual>
 <audio silent="true"/>
</toast>
)";

  ASSERT_NO_FATAL_FAILURE(VerifyXml(*notification, kExpectedXml));
}

TEST_F(NotificationTemplateBuilderTest, Images) {
  std::unique_ptr<message_center::Notification> notification =
      InitializeBasicNotification();

  SkBitmap icon;
  icon.allocN32Pixels(64, 64);
  icon.eraseARGB(255, 100, 150, 200);

  notification->set_icon(gfx::Image::CreateFrom1xBitmap(icon));
  notification->set_image(gfx::Image::CreateFrom1xBitmap(icon));

  std::vector<message_center::ButtonInfo> buttons;
  message_center::ButtonInfo button(base::ASCIIToUTF16("Button1"));
  button.type = message_center::ButtonType::TEXT;
  button.placeholder = base::ASCIIToUTF16("Reply here");
  button.icon = gfx::Image::CreateFrom1xBitmap(icon);
  buttons.emplace_back(button);
  notification->set_buttons(buttons);

  const wchar_t kExpectedXml[] =
      LR"(<toast launch="notification_id" displayTimestamp="1998-09-04T01:02:03Z">
 <visual>
  <binding template="ToastGeneric">
   <text>My Title</text>
   <text>My Message</text>
   <text placement="attribution">example.com</text>
   <image placement="appLogoOverride" src="c:\temp\img0.tmp" hint-crop="circle"/>
   <image placement="hero" src="c:\temp\img1.tmp"/>
  </binding>
 </visual>
 <actions>
  <input id="userResponse" type="text" placeHolderContent="Reply here"/>
  <action activationType="foreground" content="Button1" arguments="buttonIndex=0" imageUri="c:\temp\img2.tmp"/>
 </actions>
</toast>
)";

  ASSERT_NO_FATAL_FAILURE(VerifyXml(*notification, kExpectedXml));
}
