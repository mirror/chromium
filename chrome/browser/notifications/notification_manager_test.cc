// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifdef ENABLE_BACKGROUND_TASK
#ifdef UNIT_TEST
#include <utility>

#include "chrome/browser/notifications/notification_manager_test.h"

#include "chrome/browser/notifications/notification.h"
#include "chrome/browser/notifications/notification_manager.h"
#include "testing/gtest/include/gtest/gtest.h"

// This method is defined in the _test file so that notification_manager.cc
// has no dependencies on test files (even when UNIT_TEST is defined).
BalloonCollectionMock* NotificationManager::UseBalloonCollectionMock() {
  BalloonCollectionMock* mock = new BalloonCollectionMock;
  balloon_collection_.reset(mock);
  return mock;
}

BalloonCollectionMock::BalloonCollectionMock()
    : capacity_(0),
      count_(0),
      show_call_count_(0) {
}

BalloonCollectionMock::~BalloonCollectionMock() {
  EXPECT_EQ(0, show_call_count_) << "Not enough show calls done.";
}

void BalloonCollectionMock::Add(const Notification &notification) {
  EXPECT_GT(show_call_count_, 0) << "Unexpected show call.";
  show_call_count_--;
  count_++;
}

void BalloonCollectionMock::ShowAll() {
}

void BalloonCollectionMock::HideAll() {
}

bool BalloonCollectionMock::HasSpace() const {
  return count_ < capacity_;
}

int BalloonCollectionMock::count() const {
  return count_;
}

void BalloonCollectionMock::set_show_call_count(int show_call_count) {
  EXPECT_EQ(0, show_call_count_) << "Not enough show calls done.";
  show_call_count_ = show_call_count;
}

class UserActivityMock : public UserActivityInterface {
 public:
  UserActivityMock() : user_mode_(USER_NORMAL_MODE), observer_(NULL) {
  }

  virtual void AddObserver(UserActivityObserver* observer) {
    observer_ = observer;
  }

  virtual void CheckNow() {
  }

  virtual UserMode user_mode() const {
    return user_mode_;
  }

  virtual uint32 QueryUserIdleTimeMs() {
    return 0;
  }

  void set_user_mode(UserMode user_mode) {
    UserMode previous_user_mode = user_mode_;
    user_mode_ = user_mode;
    if (previous_user_mode != user_mode_ && observer_) {
      observer_->OnUserActivityChange();
    }
  }

 private:
  UserMode user_mode_;
  UserActivityObserver* observer_;
  DISALLOW_COPY_AND_ASSIGN(UserActivityMock);
};

TEST(NotificationManagerTest, BasicFunctionality) {
  UserActivityMock activity;
  NotificationManager manager(&activity, NULL);
  BalloonCollectionMock* balloon_collection =
      manager.UseBalloonCollectionMock();

  // Start with no room in the balloon collection.
  balloon_collection->set_capacity(0);

  // Add a notification when there is no space.
  Notification notification1(
      GURL("http://gears.google.com/MyService"),
      GURL("http://gears.google.com/MyService/notification1"));
  manager.Add(notification1);

  // Make space available and expect the balloon to be shown.
  balloon_collection->set_capacity(1);
  balloon_collection->set_show_call_count(1);
  manager.OnBalloonSpaceChanged();

  // Add a notification while the user is away.
  // Bump capacity to make sure there is space for it.
  balloon_collection->set_capacity(2);
  activity.set_user_mode(USER_AWAY_MODE);

  Notification notification2(
      GURL("http://gears.google.com/MyService"),
      GURL("http://gears.google.com/MyService/notification2"));
  manager.Add(notification2);

  // Go through all of the modes and ensure that the
  // notification doesn't get displayed.
  activity.set_user_mode(USER_IDLE_MODE);

  activity.set_user_mode(USER_INTERRUPTED_MODE);

  activity.set_user_mode(USER_MODE_UNKNOWN);

  activity.set_user_mode(USER_PRESENTATION_MODE);

  // Transitioning to normal mode should cause the notification to appear.
  balloon_collection->set_show_call_count(1);
  activity.set_user_mode(USER_NORMAL_MODE);
}

#endif  // UNIT_TEST
#endif  // ENABLE_BACKGROUND_TASK
