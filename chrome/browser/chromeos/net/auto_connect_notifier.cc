// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/net/auto_connect_notifier.h"

#include "ash/system/network/network_icon.h"
#include "ash/system/system_notifier.h"
#include "base/strings/string16.h"
#include "chrome/browser/notifications/notification_display_service.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/canvas_image_source.h"
#include "ui/gfx/image/image.h"
#include "ui/message_center/notification.h"
#include "ui/message_center/notification_types.h"
#include "ui/message_center/notifier_id.h"
#include "url/gurl.h"

namespace chromeos {

namespace {

// Signal strength to use for the notification icon. The network icon should be
// at full signal strength (4 out of 4).
const int kSignalStrength = 4;

// Dimensions of notification icon in pixels.
constexpr gfx::Size kSignalIconSize(18, 18);

const char kAutoConnectNotificationId[] =
    "cros_auto_connect_notifier_ids.connected_to_network";

}  // namespace

AutoConnectNotifier::AutoConnectNotifier(
    Profile* profile,
    AutoConnectHandler* auto_connect_handler)
    : profile_(profile), auto_connect_handler_(auto_connect_handler) {
  auto_connect_handler_->AddObserver(this);
}

AutoConnectNotifier::~AutoConnectNotifier() {
  auto_connect_handler_->RemoveObserver(this);
}

void AutoConnectNotifier::OnAutoConnectedToNetwork() {
  auto notification = std::make_unique<message_center::Notification>(
      message_center::NotificationType::NOTIFICATION_TYPE_SIMPLE,
      kAutoConnectNotificationId,
      l10n_util::GetStringUTF16(IDS_NETWORK_AUTOCONNECT_NOTIFICATION_TITLE),
      l10n_util::GetStringUTF16(IDS_NETWORK_AUTOCONNECT_NOTIFICATION_MESSAGE),
      gfx::Image() /* image */, base::string16() /* display_source */,
      GURL() /* origin_url */,
      message_center::NotifierId(
          message_center::NotifierId::NotifierType::SYSTEM_COMPONENT,
          ash::system_notifier::kNotifierNetwork),
      message_center::RichNotificationData(), nullptr /* delegate */);

  notification->SetSystemPriority();
  notification->set_small_image(
      gfx::Image(gfx::CanvasImageSource::MakeImageSkia<
                 ash::network_icon::SignalStrengthImageSource>(
          ash::network_icon::ARCS, gfx::kGoogleBlue500, kSignalIconSize,
          kSignalStrength)));

  NotificationDisplayService::GetForProfile(profile_)->Display(
      NotificationHandler::Type::TRANSIENT, *notification);
}

}  // namespace chromeos
