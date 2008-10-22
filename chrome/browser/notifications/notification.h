// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_H_
#define CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_H_

#ifdef ENABLE_BACKGROUND_TASK
#include "base/basictypes.h"
#include "googleurl/src/gurl.h"

// Representation of a notification.
class Notification {
 public:
  Notification(const GURL& origin_url, const GURL& url)
      : origin_url_(origin_url),
        url_(url) {
  }

  Notification(const Notification& notification)
      : origin_url_(notification.origin_url()),
        url_(notification.url()) {
  }

  const GURL& url() const {
    return url_;
  }

  const GURL& origin_url() const {
    return origin_url_;
  }

 private:
  GURL origin_url_;
  GURL url_;

  // Disallow assign.
  void operator=(const Notification&);
};

#endif  // ENABLE_BACKGROUND_TASK
#endif  // CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_H_
