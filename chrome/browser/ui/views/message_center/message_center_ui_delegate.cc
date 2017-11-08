// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/message_center/message_center_ui_delegate.h"

#include "chrome/browser/browser_process.h"
#include "ui/display/screen.h"
#include "ui/message_center/ui_controller.h"
#include "ui/message_center/views/desktop_popup_alignment_delegate.h"
#include "ui/message_center/views/message_popup_collection.h"

message_center::UiDelegate* CreateUiDelegate() {
  return new MessageCenterUiDelegate();
}

MessageCenterUiDelegate::MessageCenterUiDelegate() {
  ui_controller_.reset(new message_center::UiController(this));
  alignment_delegate_.reset(new message_center::DesktopPopupAlignmentDelegate);
  popup_collection_.reset(new message_center::MessagePopupCollection(
      message_center(), ui_controller_.get(), alignment_delegate_.get()));
}

MessageCenterUiDelegate::~MessageCenterUiDelegate() {
  // Reset this early so that delegated events during destruction don't cause
  // problems.
  popup_collection_.reset();
  ui_controller_.reset();
}

message_center::MessageCenter* MessageCenterUiDelegate::message_center() {
  return ui_controller_->message_center();
}

bool MessageCenterUiDelegate::ShowPopups() {
  alignment_delegate_->StartObserving(display::Screen::GetScreen());
  popup_collection_->DoUpdateIfPossible();
  return true;
}

void MessageCenterUiDelegate::HidePopups() {
  DCHECK(popup_collection_.get());
  popup_collection_->MarkAllPopupsShown();
}

bool MessageCenterUiDelegate::ShowMessageCenter(bool show_by_click) {
  // Message center not available on Windows/Linux.
  return false;
}

void MessageCenterUiDelegate::HideMessageCenter() {}

bool MessageCenterUiDelegate::ShowNotifierSettings() {
  // Message center settings not available on Windows/Linux.
  return false;
}

void MessageCenterUiDelegate::OnMessageCenterContentsChanged() {}

message_center::UiController*
MessageCenterUiDelegate::GetUiControllerForTesting() {
  return ui_controller_.get();
}
