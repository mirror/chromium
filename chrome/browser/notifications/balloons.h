// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Handles the visible notification (or balloons).

#ifndef CHROME_BROWSER_NOTIFICATIONS_BALLOONS_H_
#define CHROME_BROWSER_NOTIFICATIONS_BALLOONS_H_

#ifdef ENABLE_BACKGROUND_TASK

#include "base/basictypes.h"
#include "chrome/browser/notifications/notification.h"

class BalloonCollectionObserver {
 public:
  // Called when there is more or less space for balloons due to
  // monitor size changes or balloons disappearing.
  virtual void OnBalloonSpaceChanged() = 0;
};

class BalloonCollectionInterface {
 public:
  virtual ~BalloonCollectionInterface() {}

  // Adds a new balloon for the specified notification.
  virtual void Add(const Notification &notification) = 0;

  // Show all balloons.
  virtual void ShowAll() = 0;

  // Hide all balloons.
  virtual void HideAll() = 0;

  // Is there room to add another notification?
  virtual bool HasSpace() const = 0;

  // Number of balloons being shown.
  virtual int count() const = 0;
};

class BalloonCollection : public BalloonCollectionInterface {
 public:
  explicit BalloonCollection(BalloonCollectionObserver* observer);

  // BalloonCollectionInterface overrides
  virtual void Add(const Notification& notification);
  virtual void ShowAll();
  virtual void HideAll();
  virtual bool HasSpace() const;
  virtual int count() const { return 0; }

 private:
  BalloonCollectionObserver* observer_;
  DISALLOW_COPY_AND_ASSIGN(BalloonCollection);
};

#endif  // ENABLE_BACKGROUND_TASK

#endif  // CHROME_BROWSER_NOTIFICATIONS_BALLOONS_H_
