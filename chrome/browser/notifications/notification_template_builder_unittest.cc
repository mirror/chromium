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
#include "chrome/browser/notifications/temp_file_keeper.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/message_center/notification.h"

using message_center::Notification;
using message_center::NotifierId;
using message_center::RichNotificationData;

namespace {

const char kNotificationId[] = "notification_id";
const char kNotificationTitle[] = "My Title";
const char kNotificationMessage[] = "My Message";
const char kNotificationOrigin[] = "https://example.com";

// Intermediary format for the options available when creating a notification,
// with default values specific to this test suite to avoid endless repetition.
struct NotificationData {
  NotificationData()
      : id(kNotificationId),
        title(kNotificationTitle),
        message(kNotificationMessage),
        origin(kNotificationOrigin) {}

  std::string id;
  std::string title;
  std::string message;
  GURL origin;
};

class MockTempFileKeeper : public TempFileKeeper {
 public:
  MockTempFileKeeper() : counter_(0) {}

  base::FilePath RegisterTemporaryImage(gfx::Image image,
                                        const GURL& origin,
                                        const base::string16& prefix) override {
    base::string16 file = base::string16(L"img") +
                          base::IntToString16(counter_++) +
                          base::string16(L".tmp");
    return base::FilePath(file);
  }

 private:
  int counter_;

  DISALLOW_COPY_AND_ASSIGN(MockTempFileKeeper);
};

}  // namespace

class NotificationTemplateBuilderTest : public ::testing::Test {
 public:
  NotificationTemplateBuilderTest() = default;
  ~NotificationTemplateBuilderTest() override = default;

 protected:
  // Builds the message_center::Notification object and then the template for
  // the given |notification_data|, and writes that to |*xml_template|. Calls
  // must be wrapped in ASSERT_NO_FATAL_FAILURE().
  void BuildTemplate(const NotificationData& notification_data,
                     const std::vector<message_center::ButtonInfo>& buttons,
                     bool with_icon,
                     bool with_image,
                     base::string16* xml_template) {
    GURL origin_url(notification_data.origin);

    SkBitmap image;
    image.allocN32Pixels(64, 64);
    image.eraseARGB(255, 100, 150, 200);

    message_center::Notification notification(
        message_center::NOTIFICATION_TYPE_SIMPLE, notification_data.id,
        base::UTF8ToUTF16(notification_data.title),
        base::UTF8ToUTF16(notification_data.message),
        with_icon ? gfx::Image::CreateFrom1xBitmap(image) : gfx::Image(),
        base::string16() /* display_source */, origin_url,
        NotifierId(origin_url), RichNotificationData(), nullptr /* delegate */);
    if (buttons.size())
      notification.set_buttons(buttons);

    if (with_image)
      notification.set_image(gfx::Image::CreateFrom1xBitmap(image));

    std::unique_ptr<MockTempFileKeeper> temp_file_keeper(
        new MockTempFileKeeper());
    template_ = NotificationTemplateBuilder::Build(
        notification_data.id, *temp_file_keeper, notification);
    ASSERT_TRUE(template_);

    *xml_template = template_->GetNotificationTemplate();
  }

 protected:
  std::unique_ptr<NotificationTemplateBuilder> template_;

 private:
  DISALLOW_COPY_AND_ASSIGN(NotificationTemplateBuilderTest);
};

TEST_F(NotificationTemplateBuilderTest, SimpleToast) {
  NotificationData notification_data;
  base::string16 xml_template;
  std::vector<message_center::ButtonInfo> buttons;

  ASSERT_NO_FATAL_FAILURE(
      BuildTemplate(notification_data, buttons, false, false, &xml_template));

  const wchar_t kExpectedXml[] =
      LR"(<toast launch="notification_id">
 <visual>
  <binding template="ToastGeneric">
   <text>My Title</text>
   <text>My Message</text>
   <text placement="attribution">example.com</text>
  </binding>
 </visual>
</toast>
)";

  EXPECT_EQ(xml_template, kExpectedXml);
}

TEST_F(NotificationTemplateBuilderTest, Buttons) {
  NotificationData notification_data;
  base::string16 xml_template;

  std::vector<message_center::ButtonInfo> buttons;
  buttons.emplace_back(base::ASCIIToUTF16("Button1"));
  buttons.emplace_back(base::ASCIIToUTF16("Button2"));

  ASSERT_NO_FATAL_FAILURE(
      BuildTemplate(notification_data, buttons, false, false, &xml_template));

  const wchar_t kExpectedXml[] =
      LR"(<toast launch="notification_id">
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

  EXPECT_EQ(xml_template, kExpectedXml);
}

TEST_F(NotificationTemplateBuilderTest, InlineReplies) {
  NotificationData notification_data;
  base::string16 xml_template;

  std::vector<message_center::ButtonInfo> buttons;
  message_center::ButtonInfo button1(base::ASCIIToUTF16("Button1"));
  button1.type = message_center::ButtonType::TEXT;
  button1.placeholder = base::ASCIIToUTF16("Reply here");
  buttons.emplace_back(button1);
  buttons.emplace_back(base::ASCIIToUTF16("Button2"));

  ASSERT_NO_FATAL_FAILURE(
      BuildTemplate(notification_data, buttons, false, false, &xml_template));

  const wchar_t kExpectedXml[] =
      LR"(<toast launch="notification_id">
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

  EXPECT_EQ(xml_template, kExpectedXml);
}

TEST_F(NotificationTemplateBuilderTest, InlineRepliesDoubleInput) {
  NotificationData notification_data;
  base::string16 xml_template;

  std::vector<message_center::ButtonInfo> buttons;
  message_center::ButtonInfo button1(base::ASCIIToUTF16("Button1"));
  button1.type = message_center::ButtonType::TEXT;
  button1.placeholder = base::ASCIIToUTF16("Reply here");
  buttons.emplace_back(button1);
  message_center::ButtonInfo button2(base::ASCIIToUTF16("Button2"));
  button2.type = message_center::ButtonType::TEXT;
  button2.placeholder = base::ASCIIToUTF16("Should not appear");
  buttons.emplace_back(button2);

  ASSERT_NO_FATAL_FAILURE(
      BuildTemplate(notification_data, buttons, false, false, &xml_template));

  const wchar_t kExpectedXml[] =
      LR"(<toast launch="notification_id">
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

  EXPECT_EQ(xml_template, kExpectedXml);
}

TEST_F(NotificationTemplateBuilderTest, InlineRepliesTextTypeNotFirst) {
  NotificationData notification_data;
  base::string16 xml_template;

  std::vector<message_center::ButtonInfo> buttons;
  buttons.emplace_back(base::ASCIIToUTF16("Button1"));
  message_center::ButtonInfo button2(base::ASCIIToUTF16("Button2"));
  button2.type = message_center::ButtonType::TEXT;
  button2.placeholder = base::ASCIIToUTF16("Reply here");
  buttons.emplace_back(button2);

  ASSERT_NO_FATAL_FAILURE(
      BuildTemplate(notification_data, buttons, false, false, &xml_template));

  const wchar_t kExpectedXml[] =
      LR"(<toast launch="notification_id">
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

  EXPECT_EQ(xml_template, kExpectedXml);
}

TEST_F(NotificationTemplateBuilderTest, Images) {
  NotificationData notification_data;
  base::string16 xml_template;

  SkBitmap icon;
  icon.allocN32Pixels(64, 64);
  icon.eraseARGB(255, 100, 150, 200);

  std::vector<message_center::ButtonInfo> buttons;
  message_center::ButtonInfo button(base::ASCIIToUTF16("Button1"));
  button.type = message_center::ButtonType::TEXT;
  button.placeholder = base::ASCIIToUTF16("Reply here");
  button.icon = gfx::Image::CreateFrom1xBitmap(icon);
  buttons.emplace_back(button);

  ASSERT_NO_FATAL_FAILURE(
      BuildTemplate(notification_data, buttons, true, true, &xml_template));

  const wchar_t kExpectedXml[] =
      LR"(<toast launch="notification_id">
 <visual>
  <binding template="ToastGeneric">
   <text>My Title</text>
   <text>My Message</text>
   <text placement="attribution">example.com</text>
   <image placement="appLogoOverride" src="img0.tmp" hint-crop="circle"/>
   <image placement="hero" src="img1.tmp"/>
  </binding>
 </visual>
 <actions>
  <input id="userResponse" type="text" placeHolderContent="Reply here"/>
  <action activationType="foreground" content="Button1" arguments="buttonIndex=0" imageUri="img2.tmp"/>
 </actions>
</toast>
)";

  EXPECT_EQ(xml_template, kExpectedXml);
}
