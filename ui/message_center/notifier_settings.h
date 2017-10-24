// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_MESSAGE_CENTER_NOTIFIER_SETTINGS_H_
#define UI_MESSAGE_CENTER_NOTIFIER_SETTINGS_H_

#include <stddef.h>

#include <string>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/strings/string16.h"
#include "ui/gfx/image/image.h"
#include "ui/message_center/message_center_export.h"
#include "url/gurl.h"

class MessageCenterNotificationsTest;
class MessageCenterTrayBridgeTest;

namespace ash {
class WebNotificationTrayTest;
}

namespace message_center {
namespace test {
class MessagePopupCollectionTest;
}

class MessageCenterNotificationManagerTest;
class Notification;
class NotifierSettingsDelegate;

// A struct that identifies the source of notifications. For example, a web page
// might send multiple notifications but they'd all have the same NotifierId.
// TODO(estade): rename to Notifier.
struct MESSAGE_CENTER_EXPORT NotifierId {
  // This enum is being used for histogram reporting and the elements should not
  // be re-ordered.
  enum NotifierType : int {
    APPLICATION = 0,
    ARC_APPLICATION = 1,
    WEB_PAGE = 2,
    SYSTEM_COMPONENT = 3,
    SIZE,
  };

  // Constructor for non WEB_PAGE type.
  NotifierId(NotifierType type, const std::string& id);

  // Constructor for WEB_PAGE type.
  explicit NotifierId(const GURL& url);

  NotifierId(const NotifierId& other);

  bool operator==(const NotifierId& other) const;
  // Allows NotifierId to be used as a key in std::map.
  bool operator<(const NotifierId& other) const;

  NotifierType type;

  // The identifier of the app notifier. Empty if it's WEB_PAGE.
  std::string id;

  // The URL pattern of the notifer.
  GURL url;

  // The identifier of the profile where the notification is created. This is
  // used for ChromeOS multi-profile support and can be empty.
  std::string profile_id;

 private:
  friend class MessageCenterNotificationManagerTest;
  friend class MessageCenterTrayTest;
  friend class Notification;
  friend class NotificationControllerTest;
  friend class PopupCollectionTest;
  friend class TrayViewControllerTest;
  friend class ::MessageCenterNotificationsTest;
  friend class ::MessageCenterTrayBridgeTest;
  friend class ash::WebNotificationTrayTest;
  friend class test::MessagePopupCollectionTest;
  FRIEND_TEST_ALL_PREFIXES(PopupControllerTest, Creation);
  FRIEND_TEST_ALL_PREFIXES(NotificationListTest, UnreadCountNoNegative);
  FRIEND_TEST_ALL_PREFIXES(NotificationListTest, TestHasNotificationOfType);

  // The default constructor which doesn't specify the notifier. Used for tests.
  NotifierId();
};

// A struct to hold UI information about notifiers. The data is used by
// NotifierSettingsView.
// FIXME: move to ash.
struct MESSAGE_CENTER_EXPORT NotifierUiData {
  NotifierUiData(const NotifierId& notifier_id,
                 const base::string16& name,
                 bool has_advanced_settings,
                 bool enabled);
  ~NotifierUiData();

  NotifierId notifier_id;

  // The human-readable name of the notifier such like the extension name.
  // It can be empty.
  base::string16 name;

  // True if the notifier should have an affordance for advanced settings.
  bool has_advanced_settings;

  // True if the source is allowed to send notifications. True is default.
  bool enabled;

  // The icon image of the notifier. The extension icon or favicon.
  gfx::Image icon;

 private:
  DISALLOW_COPY_AND_ASSIGN(NotifierUiData);
};

}  // namespace message_center

#endif  // UI_MESSAGE_CENTER_NOTIFIER_SETTINGS_H_
