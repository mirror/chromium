// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_NOTIFICATIONS_NOTIFICATION_ID_GENERATOR_H_
#define CONTENT_BROWSER_NOTIFICATIONS_NOTIFICATION_ID_GENERATOR_H_

#include <stdint.h>
#include <string>

#include "base/macros.h"
#include "base/strings/string_piece.h"
#include "content/common/content_export.h"
#include "url/origin.h"

class GURL;

namespace content {

// Generates deterministic notification ids for Web Notifications.
//
// The notification id must be deterministic for a given origin and tag, when
// the tag is non-empty, or unique for the given notification when the tag is
// empty. For non-persistent notifications, the uniqueness will be based on the
// render process id. For persistent notifications, the generated id will be
// globally unique for the lifetime of the notification database.
//
// Notifications coming from the same origin and having the same tag will result
// in the same notification id being generated. This id may then be used to
// update the notification in the platform notification service.
//
// The notification id will be used by the notification service for determining
// when to replace notifications, and as the unique identifier when a
// notification has to be closed programmatically.
//
// It is important to note that, for persistent notifications, the generated
// notification id can outlive the browser process responsible for creating it.
//
// Note that the PlatformNotificationService is expected to handle
// distinguishing identical generated ids from different browser contexts.
//
// Also note that several functions in NotificationPlatformBridge class
// rely on the format of the notification generated here.
// Code: chrome/android/java/src/org/chromium/chrome/browser/notifications/
// NotificationPlatformBridge.java
class CONTENT_EXPORT NotificationIdGenerator {
 public:
  NotificationIdGenerator() = default;

  // Returns whether |notification_id| belongs to a persistent notification.
  static bool IsPersistentNotification(
      const base::StringPiece& notification_id);

  // Returns whether |notification_id| belongs to a non-persistent notification.
  static bool IsNonPersistentNotification(
      const base::StringPiece& notification_id);

  // Generates an id for a persistent notification given the notification's
  // origin, tag and persistent notification id. The persistent notification id
  // will have been created by the persistent notification database.
  std::string GenerateForPersistentNotification(
      const GURL& origin,
      const std::string& tag,
      int64_t persistent_notification_id) const;

  // Generates an id for a non-persistent notification given the notification's
  // origin, tag and request id. The request id must've been created by the
  // |render_process_id|.
  // TODO(https://crbug.com/595685): Remove this method once we use the mojo
  // pathway by default.
  std::string GenerateForNonPersistentNotification(const GURL& origin,
                                                   const std::string& tag,
                                                   int request_id,
                                                   int render_process_id) const;

  // Generates an id for a non-persistent notification given the notification's
  // |origin| and |token|.
  //
  // |token| is what determines which notifications from the same origin receive
  // the same notification ID and therefore which notifications will replace
  // each other. (So different notifications with the same non-empty tag should
  // have the same token, but notifications without tags should have unique
  // tokens.)
  // TODO(https://crbug.com/595685): Remove 'Mojo' from this method name once
  // we use the mojo pathway by default.
  std::string GenerateForNonPersistentMojoNotification(
      const url::Origin& origin,
      const std::string& token) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(NotificationIdGenerator);
};

}  // namespace context

#endif  // CONTENT_BROWSER_NOTIFICATIONS_NOTIFICATION_ID_GENERATOR_H_
