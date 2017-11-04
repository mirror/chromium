// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OPTIMIZATION_GUIDE_NOTIFICATION_TYPES_H
#define COMPONENTS_OPTIMIZATION_GUIDE_NOTIFICATION_TYPES_H

namespace optimization_guide {

// Notification types that the Optimization Guide Service dispatches.
// All notifications will be dispatched on the IO thread.
enum NotificationType {
  // Special signal used by the NotificationService to represent an interest in
  // all notifications.
  // Not valid when posting a notificiation.
  NOTIFICATION_ALL = 0,

  // Notification dispatched when the hints downloaded from the Optimization
  // Hints component have been validated and ready for further processing.
  NOTIFICATION_HINTS_READY,

};

}  // namespace optimization_guide

#endif  // COMPONENTS_OPTIMIZATION_GUIDE_NOTIFICATION_TYPES_H
