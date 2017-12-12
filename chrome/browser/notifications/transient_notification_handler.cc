// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/notifications/transient_notification_handler.h"

#include "base/callback.h"
#include "chrome/browser/notifications/notification_display_service.h"

TransientNotificationHandler::TransientNotificationHandler(
    NotificationDisplayService* service)
    : service_(service) {}

TransientNotificationHandler::~TransientNotificationHandler() = default;

void TransientNotificationHandler::OnShow(Profile* profile,
                                          const std::string& notification_id) {
  transient_notifications_.insert(notification_id);
}

// TODO(peter): We need to listen to OnClose() to remove notifications from
// the |transient_notifications_| set, but that implies overriding the delegate
// to one of our own.

void TransientNotificationHandler::Shutdown() {
  for (const std::string& notification_id : transient_notifications_)
    service_->Close(NotificationHandler::Type::TRANSIENT, notification_id);
}
