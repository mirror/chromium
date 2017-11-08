// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_MESSAGE_CENTER_MESSAGE_CENTER_UI_DELEGATE_H_
#define CHROME_BROWSER_UI_VIEWS_MESSAGE_CENTER_MESSAGE_CENTER_UI_DELEGATE_H_

#include <memory>

#include "base/macros.h"
#include "ui/message_center/ui_delegate.h"

namespace message_center {
class DesktopPopupAlignmentDelegate;
class MessageCenter;
class UiController;
class MessagePopupCollection;
}  // namespace message_center

// A UiDelegate implementation that exposes the UiController via a system tray
// icon. The notification popups (toasts) will be displayed in the corner of the
// screen but there is no dedicated message center.
class MessageCenterUiDelegate : public message_center::UiDelegate {
 public:
  MessageCenterUiDelegate();
  ~MessageCenterUiDelegate() override;

  message_center::MessageCenter* message_center();

  // UiDelegate implementation.
  bool ShowPopups() override;
  void HidePopups() override;
  bool ShowMessageCenter(bool show_by_click) override;
  void HideMessageCenter() override;
  void OnMessageCenterContentsChanged() override;
  bool ShowNotifierSettings() override;

  message_center::UiController* GetUiControllerForTesting();

 private:
  std::unique_ptr<message_center::MessagePopupCollection> popup_collection_;
  std::unique_ptr<message_center::DesktopPopupAlignmentDelegate>
      alignment_delegate_;
  std::unique_ptr<message_center::UiController> ui_controller_;

  DISALLOW_COPY_AND_ASSIGN(MessageCenterUiDelegate);
};

#endif  // CHROME_BROWSER_UI_VIEWS_MESSAGE_CENTER_MESSAGE_CENTER_UI_DELEGATE_H_
