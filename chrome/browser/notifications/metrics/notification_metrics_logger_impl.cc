// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/notifications/metrics/notification_metrics_logger_impl.h"

#include "base/metrics/histogram_macros.h"
#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"
#include "chrome/browser/notifications/metrics/notification_metrics_logger_factory.h"
#include "content/public/common/persistent_notification_status.h"

NotificationMetricsLoggerImpl::NotificationMetricsLoggerImpl() = default;

NotificationMetricsLoggerImpl::~NotificationMetricsLoggerImpl() = default;

void NotificationMetricsLoggerImpl::LogPersistentNotificationClosedByUser() {
  base::RecordAction(
      base::UserMetricsAction("Notifications.Persistent.ClosedByUser"));
}

void NotificationMetricsLoggerImpl::
    LogPersistentNotificationClosedProgrammatically() {
  base::RecordAction(base::UserMetricsAction(
      "Notifications.Persistent.ClosedProgrammatically"));
}

void NotificationMetricsLoggerImpl::
    LogPersistentNotificationActionButtonClick() {
  base::RecordAction(
      base::UserMetricsAction("Notifications.Persistent.ClickedActionButton"));
}

void NotificationMetricsLoggerImpl::LogPersistentNotificationClick() {
  base::RecordAction(
      base::UserMetricsAction("Notifications.Persistent.Clicked"));
}

void NotificationMetricsLoggerImpl::LogPersistentNotificationClickResult(
    content::PersistentNotificationStatus status) {
  UMA_HISTOGRAM_ENUMERATION(
      "Notifications.PersistentWebNotificationClickResult", status,
      content::PersistentNotificationStatus::
          PERSISTENT_NOTIFICATION_STATUS_MAX);
}

void NotificationMetricsLoggerImpl::LogPersistentNotificationCloseResult(
    content::PersistentNotificationStatus status) {
  UMA_HISTOGRAM_ENUMERATION(
      "Notifications.PersistentWebNotificationCloseResult", status,
      content::PersistentNotificationStatus::
          PERSISTENT_NOTIFICATION_STATUS_MAX);
}

void NotificationMetricsLoggerImpl::
    LogPersistentNotificationClickWithoutPermission() {
  base::RecordAction(base::UserMetricsAction(
      "Notifications.Persistent.ClickedWithoutPermission"));
}

void NotificationMetricsLoggerImpl::LogPersistentNotificationShown() {
  base::RecordAction(base::UserMetricsAction("Notifications.Persistent.Shown"));
}
