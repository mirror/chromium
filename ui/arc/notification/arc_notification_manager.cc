// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/arc/notification/arc_notification_manager.h"

#include "base/stl_util.h"
#include "ui/arc/notification/arc_notification_item.h"

namespace arc {

ArcNotificationManager::ArcNotificationManager(ArcBridgeService* bridge_service,
                                               const AccountId& main_profile_id)
    : ArcService(bridge_service),
      main_profile_id_(main_profile_id),
      binding_(this) {
  arc_bridge_service()->AddObserver(this);
}

ArcNotificationManager::~ArcNotificationManager() {
  arc_bridge_service()->RemoveObserver(this);
}

void ArcNotificationManager::OnNotificationsInstanceReady() {
  NotificationsInstance* notifications_instance =
      arc_bridge_service()->notifications_instance();
  if (!notifications_instance) {
    VLOG(2) << "Request to refresh app list when bridge service is not ready.";
    return;
  }

  NotificationsHostPtr host;
  binding_.Bind(mojo::GetProxy(&host));
  notifications_instance->Init(std::move(host));
}

void ArcNotificationManager::OnNotificationPosted(ArcNotificationDataPtr data) {
  ArcNotificationItem* item = items_.get(data->key);
  if (!item) {
    // Show a notification on the primary loged-in user's desktop.
    // TODO(yoshiki): Reconsider when ARC supports multi-user.
    item = new ArcNotificationItem(this, message_center::MessageCenter::Get(),
                                   data->key, main_profile_id_);
    items_.set(data->key, make_scoped_ptr(item));
  }
  item->UpdateWithArcNotificationData(*data);
}

void ArcNotificationManager::OnNotificationRemoved(const mojo::String& key) {
  ItemMap::iterator it = items_.find(key.get());
  if (it == items_.end()) {
    VLOG(3) << "Android requests to remove a notification (key: " << key
            << "), but it is already gone.";
    return;
  }

  scoped_ptr<ArcNotificationItem> item(items_.take_and_erase(it));
  item->OnClosedFromAndroid();
}

void ArcNotificationManager::SendNotificationRemovedFromChrome(
    const std::string& key) {
  ItemMap::iterator it = items_.find(key);
  if (it == items_.end()) {
    VLOG(3) << "Chrome requests to remove a notification (key: " << key
            << "), but it is already gone.";
    return;
  }

  scoped_ptr<ArcNotificationItem> item(items_.take_and_erase(it));

  arc_bridge_service()
      ->notifications_instance()
      ->SendNotificationEventToAndroid(key, ArcNotificationEvent::CLOSED);
}

void ArcNotificationManager::SendNotificationClickedOnChrome(
    const std::string& key) {
  if (!items_.contains(key)) {
    VLOG(3) << "Chrome requests to fire a click event on notification (key: "
            << key << "), but it is gone.";
    return;
  }

  arc_bridge_service()
      ->notifications_instance()
      ->SendNotificationEventToAndroid(key, ArcNotificationEvent::BODY_CLICKED);
}

}  // namespace arc
