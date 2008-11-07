// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_MANAGER_TEST_H_
#define CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_MANAGER_TEST_H_

#ifdef ENABLE_BACKGROUND_TASK
#ifdef UNIT_TEST

#include <map>

#include "chrome/browser/notifications/balloons.h"
#include "chrome/browser/notifications/notification.h"
#include "googleurl/src/gurl.h"

class BalloonCollectionMock : public BalloonCollectionInterface {
 public:
  BalloonCollectionMock();
  virtual ~BalloonCollectionMock();

  virtual void Add(const Notification& notification,
                   Profile* profile,
                   SiteInstance* site_instance);
  virtual void ShowAll();
  virtual void HideAll();

  virtual bool HasSpace() const;
  virtual int count() const;

  void set_capacity(int capacity) {
    capacity_ = capacity;
  }

  int show_call_count() const {
    return show_call_count_;
  }

  void set_show_call_count(int show_call_count);

 private:
  int capacity_;
  int count_;
  int show_call_count_;

  DISALLOW_COPY_AND_ASSIGN(BalloonCollectionMock);
};

#endif  // UNIT_TEST
#endif  // ENABLE_BACKGROUND_TASK

#endif  // CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_MANAGER_TEST_H_
