// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_PERMISSION_CONTEXT_H_
#define CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_PERMISSION_CONTEXT_H_

#include "base/gtest_prod_util.h"
#include "chrome/browser/permissions/permission_context_base.h"
#include "components/content_settings/core/common/content_settings.h"

class GURL;
class Profile;

// The NotificationPermissionContext determines whether an origin can use Web
// Notifications and the Push API.
//
// REQUESTING PERMISSION
//
// Only top-level secure origins may request permission. This is implemented by
// DecidePermission() and IsRestrictedToSecureOrigins(). Both checks are
// duplicated in Blink to avoid a synchronous IPC call for supporting the
// Notification.permission property.
//
// Permission is not granted in Incognito. Requests are automatically denied
// after a delay of between [1.0, 2.0) seconds, so that sites cannot detect
// Incognito users.
//
// On Android, we automatically deny permission requests when no previous
// decision has been made, and notifications have been blocked for the whole
// browser app. In that case it does not make sense to request (let alone grant)
// the permission, as notifications cannot be shown.
//
// GETTING PERMISSION STATUS
//
// When the requesting and embedding origin differ, the notification permission
// will only return ALLOW or BLOCK as the permission status, never ASK. This is
// because developers may not request permission from cross-origin iFrames.
class NotificationPermissionContext : public PermissionContextBase {
 public:
  NotificationPermissionContext(Profile* profile, ContentSettingsType);
  ~NotificationPermissionContext() override;

  // PermissionContextBase implementation.
  ContentSetting GetPermissionStatusInternal(
      content::RenderFrameHost* render_frame_host,
      const GURL& requesting_origin,
      const GURL& embedding_origin) const override;
  void ResetPermission(const GURL& requesting_origin,
                       const GURL& embedder_origin) override;
  void CancelPermissionRequest(content::WebContents* web_contents,
                               const PermissionRequestID& id) override;

 private:
  FRIEND_TEST_ALL_PREFIXES(NotificationPermissionContextTest,
                           PushTopLevelOriginOnly);
  friend class NotificationPermissionContextTest;

  // PermissionContextBase implementation.
  void DecidePermission(content::WebContents* web_contents,
                        const PermissionRequestID& id,
                        const GURL& requesting_origin,
                        const GURL& embedding_origin,
                        bool user_gesture,
                        const BrowserPermissionCallback& callback) override;
  void UpdateContentSetting(const GURL& requesting_origin,
                            const GURL& embedder_origin,
                            ContentSetting content_setting) override;
  bool IsRestrictedToSecureOrigins() const override;

  base::WeakPtrFactory<NotificationPermissionContext> weak_factory_ui_thread_;
};

#endif  // CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_PERMISSION_CONTEXT_H_
