// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/cocoa/notifications/ui_controller_bridge.h"

#include "base/bind.h"
#include "base/i18n/number_formatting.h"
#include "base/message_loop/message_loop.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/notifications/message_center_notification_manager.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#import "ui/message_center/cocoa/popup_collection.h"
#include "ui/message_center/message_center.h"
#include "ui/message_center/ui_controller.h"

message_center::UiDelegate* CreateUiDelegate() {
  return new UiControllerBridge(g_browser_process->message_center());
}

UiControllerBridge::UiControllerBridge(
    message_center::MessageCenter* message_center)
    : message_center_(message_center),
      controller_(new message_center::UiController(this, message_center)) {}

UiControllerBridge::~UiControllerBridge() {}

void UiControllerBridge::OnMessageCenterContentsChanged() {}

bool UiControllerBridge::ShowPopups() {
  popup_collection_.reset(
      [[MCPopupCollection alloc] initWithMessageCenter:message_center_]);
  return true;
}

void UiControllerBridge::HidePopups() {
  popup_collection_.reset();
}

bool UiControllerBridge::ShowMessageCenter(bool show_by_click) {
  return false;
}

void UiControllerBridge::HideMessageCenter() {}

bool UiControllerBridge::ShowNotifierSettings() {
  return false;
}
