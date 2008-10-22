// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifdef ENABLE_BACKGROUND_TASK
#include "chrome/browser/notifications/notification_manager.h"

#include "base/logging.h"
#include "chrome/browser/notifications/balloons.h"
#include "chrome/browser/notifications/notification.h"
#include "chrome/browser/notifications/user_activity.h"

typedef std::deque<Notification*> QueuedNotifications;

void Clear(QueuedNotifications* notifications) {
  while (!notifications->empty()) {
    QueuedNotifications::reverse_iterator it =
        notifications->rbegin();
    Notification* removed = *it;
    notifications->pop_back();
    delete removed;
  }
}

NotificationManager::NotificationManager(
    UserActivityInterface *activity,
    BalloonCollectionObserver *balloons_observer)
    : activity_(activity),
      balloons_observer_(balloons_observer),
      in_presentation_(false) {
  assert(activity);
  activity->AddObserver(this);
  balloon_collection_.reset(new BalloonCollection(this));
}

NotificationManager::~NotificationManager() {
  Clear(&show_queue_);
}

void NotificationManager::Add(const Notification& notification) {
  LOG(INFO) << "Added notification. url: "
	        << notification.url().spec().c_str();
  show_queue_.push_back(new Notification(notification));
  CheckAndShowNotifications();
}

void NotificationManager::CheckAndShowNotifications() {
  // Is it ok to show the notification now?
  activity_->CheckNow();
  if (!IsActiveUserMode(activity_->user_mode())) {
    return;
  }

  ShowNotifications();
}

void NotificationManager::ShowNotifications() {
  while (!show_queue_.empty() && balloon_collection_->HasSpace()) {
    Notification* notification = show_queue_.front();
    show_queue_.pop_front();
    balloon_collection_->Add(*notification);
    delete notification;
  }
}

void NotificationManager::OnUserActivityChange() {
  bool previous_in_presentation = in_presentation_;
  UserMode user_mode = activity_->user_mode();
  in_presentation_ = user_mode == USER_PRESENTATION_MODE;
  if (previous_in_presentation != in_presentation_) {
    if (in_presentation_) {
      balloon_collection_->HideAll();
    } else {
      balloon_collection_->ShowAll();
    }
  }

  if (IsActiveUserMode(user_mode)) {
    ShowNotifications();
  }
}

void NotificationManager::OnBalloonSpaceChanged() {
  CheckAndShowNotifications();
  if (balloons_observer_) {
    balloons_observer_->OnBalloonSpaceChanged();
  }
}

bool NotificationManager::showing_notifications() const {
  return balloon_collection_.get() && balloon_collection_->count() > 0;
}

#endif  // ENABLE_BACKGROUND_TASK
