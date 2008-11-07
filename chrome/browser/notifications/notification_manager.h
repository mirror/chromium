// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_MANAGER_H_
#define CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_MANAGER_H_

#ifdef ENABLE_BACKGROUND_TASK

#include <deque>

#include "base/scoped_ptr.h"
#include "chrome/browser/notifications/balloons.h"
#include "chrome/browser/notifications/user_activity.h"

#ifdef UNIT_TEST
class BalloonCollectionMock;
#endif  // UNIT_TEST
class Notification;
struct NotificationManagerSingletonTraits;
class Profile;

class QueuedNotification;
typedef std::deque<QueuedNotification*> QueuedNotifications;

class SiteInstance;

// Handles all aspects of the notifications to be displayed.
class NotificationManager : public BalloonCollectionObserver,
                            public UserActivityObserver {
 public:
  // Gets the singleton NotificationManager object.
  static NotificationManager* GetInstance();

  NotificationManager(UserActivityInterface* activity,
                      BalloonCollectionObserver* balloons_observer);
  ~NotificationManager();

  // Adds a notification to be displayed.
  void Add(const Notification& notification,
           Profile* profile,
           SiteInstance* site_instance);

  // BalloonCollectionObserver implementation.
  virtual void OnBalloonSpaceChanged();

  // UserActivityObserver implementation.
  void OnUserActivityChange();

  // Are we showing any notifications?
  bool showing_notifications() const;

  // Do we have any pending notifications to show?
  bool has_pending_notifications() const;

#ifdef UNIT_TEST
  BalloonCollectionMock* UseBalloonCollectionMock();
#endif  // UNIT_TEST

 private:
  friend struct NotificationManagerSingletonTraits;

  // Attempts to display notifications from the show_queue if the user
  // is active.
  void CheckAndShowNotifications();

  // Attempts to display notifications from the show_queue.
  void ShowNotifications();

  UserActivityInterface* activity_;
  BalloonCollectionObserver* balloons_observer_;
  scoped_ptr<BalloonCollectionInterface> balloon_collection_;
  QueuedNotifications show_queue_;
  bool in_presentation_;
  DISALLOW_COPY_AND_ASSIGN(NotificationManager);
};

#endif  // ENABLE_BACKGROUND_TASK
#endif  // CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_MANAGER_H_
