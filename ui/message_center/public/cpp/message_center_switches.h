// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_MESSAGE_CENTER_PUBLIC_CPP_MESSAGE_CENTER_SWITCHES_H_
#define UI_MESSAGE_CENTER_PUBLIC_CPP_MESSAGE_CENTER_SWITCHES_H_

#include "base/feature_list.h"
#include "ui/message_center/public/cpp/message_center_public_export.h"

namespace message_center {

// This feature controls whether the new (material design) style notifications
// should be used.
extern const base::Feature kNewStyleNotifications;

// Returns if new style notification is enabled, i.e. NotificationViewMD is
// used instead of NotificationView.
bool MESSAGE_CENTER_PUBLIC_EXPORT IsNewStyleNotificationEnabled();

}  // namespace message_center

#endif  // UI_MESSAGE_CENTER_PUBLIC_CPP_MESSAGE_CENTER_SWITCHES_H_
