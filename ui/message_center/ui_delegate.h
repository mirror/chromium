// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_MESSAGE_CENTER_UI_DELEGATE_H_
#define UI_MESSAGE_CENTER_UI_DELEGATE_H_

#include "ui/message_center/message_center_export.h"

namespace message_center {

// A UiDelegate class is responsible for managing the various UI surfaces
// (MessageCenter and popups) as notifications are added and updated.
// Implementations are platform specific.
class MESSAGE_CENTER_EXPORT UiDelegate {
 public:
  virtual ~UiDelegate(){};

  // Called whenever a change to the visible UI has occurred.
  virtual void OnMessageCenterContentsChanged() = 0;

  // Display the popup bubbles for new notifications to the user.  Returns true
  // if popups were actually displayed to the user.
  virtual bool ShowPopups() = 0;

  // Remove the popup bubbles from the UI.
  virtual void HidePopups() = 0;

  // Display the notifier settings as a bubble.
  virtual void ShowNotifierSettings() = 0;

  virtual bool IsMessageCenterVisible() = 0;
};

}  // namespace message_center

#endif  // UI_MESSAGE_CENTER_UI_DELEGATE_H_
