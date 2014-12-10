// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/browser/layout_test/layout_test_notification_manager.h"

#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/desktop_notification_delegate.h"
#include "content/public/common/show_desktop_notification_params.h"

namespace content {

LayoutTestNotificationManager::LayoutTestNotificationManager()
    : weak_factory_(this) {}

LayoutTestNotificationManager::~LayoutTestNotificationManager() {}

blink::WebNotificationPermission
LayoutTestNotificationManager::CheckPermission(
    ResourceContext* resource_context,
    const GURL& origin,
    int render_process_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  NotificationPermissionMap::iterator iter =
      permission_map_.find(origin);
  if (iter == permission_map_.end())
    return blink::WebNotificationPermissionDefault;

  return iter->second;
}

bool LayoutTestNotificationManager::RequestPermission(const GURL& origin) {
  return CheckPermission(nullptr, origin, 0) ==
      blink::WebNotificationPermissionAllowed;
}

void LayoutTestNotificationManager::SetPermission(
    const GURL& origin,
    blink::WebNotificationPermission permission) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  permission_map_[origin] = permission;
}

void LayoutTestNotificationManager::ClearPermissions() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  permission_map_.clear();
}

void LayoutTestNotificationManager::DisplayNotification(
    BrowserContext* browser_context,
    const ShowDesktopNotificationHostMsgParams& params,
    scoped_ptr<DesktopNotificationDelegate> delegate,
    int render_process_id,
    base::Closure* cancel_callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  std::string title = base::UTF16ToUTF8(params.title);

  DCHECK(cancel_callback);
  *cancel_callback = base::Bind(&LayoutTestNotificationManager::Close,
                                weak_factory_.GetWeakPtr(),
                                title);

  if (params.replace_id.length()) {
    std::string replace_id = base::UTF16ToUTF8(params.replace_id);
    auto replace_iter = replacements_.find(replace_id);
    if (replace_iter != replacements_.end()) {
      const std::string& previous_title = replace_iter->second;
      auto notification_iter = notifications_.find(previous_title);
      if (notification_iter != notifications_.end()) {
        DesktopNotificationDelegate* previous_delegate =
            notification_iter->second;
        previous_delegate->NotificationClosed(false);

        notifications_.erase(notification_iter);
        delete previous_delegate;
      }
    }

    replacements_[replace_id] = title;
  }

  notifications_[title] = delegate.release();
  notifications_[title]->NotificationDisplayed();
}

void LayoutTestNotificationManager::DisplayPersistentNotification(
    BrowserContext* browser_context,
    int64 service_worker_registration_id,
    const ShowDesktopNotificationHostMsgParams& params,
    int render_process_id) {
  // TODO(peter): Make persistent notifications layout testable.
  NOTIMPLEMENTED();
}

void LayoutTestNotificationManager::ClosePersistentNotification(
    BrowserContext* browser_context,
    const std::string& persistent_notification_id) {
  // TODO(peter): Make persistent notifications layout testable.
  NOTIMPLEMENTED();
}

void LayoutTestNotificationManager::SimulateClick(const std::string& title) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  auto iterator = notifications_.find(title);
  if (iterator == notifications_.end())
    return;

  iterator->second->NotificationClick();
}

void LayoutTestNotificationManager::Close(const std::string& title) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  auto iterator = notifications_.find(title);
  if (iterator == notifications_.end())
    return;

  iterator->second->NotificationClosed(false);
}

}  // namespace content
