// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/system_notifier.h"

#include "ash/public/interfaces/notification_constants.mojom.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "ui/message_center/public/cpp/message_center_switches.h"

namespace ash {
namespace system_notifier {

namespace {

// See http://dev.chromium.org/chromium-os/chromiumos-design-docs/
// system-notifications for the reasoning.

// |kAlwaysShownSystemNotifierIds| is the list of system notification sources
// which can appear regardless of the situation, such like login screen or lock
// screen.
const char* kAlwaysShownSystemNotifierIds[] = {
    mojom::kNotifierAccessibility, mojom::kNotifierDeprecatedAccelerator,
    mojom::kNotifierBattery, mojom::kNotifierDisplay,
    mojom::kNotifierDisplayError, mojom::kNotifierNetworkError,
    mojom::kNotifierPower, mojom::kNotifierStylusBattery,
    // Note: Order doesn't matter here, so keep this in alphabetic order, don't
    // just add your stuff at the end!
    NULL};

// |kAshSystemNotifiers| is the list of normal system notification sources for
// ash events. These notifications can be hidden in some context.
const char* kAshSystemNotifiers[] = {
    mojom::kNotifierBluetooth, mojom::kNotifierCapsLock,
    mojom::kNotifierDisplayResolutionChange, mojom::kNotifierDisk,
    mojom::kNotifierLocale, mojom::kNotifierMultiProfileFirstRun,
    mojom::kNotifierNetwork, mojom::kNotifierNetworkPortalDetector,
    mojom::kNotifierScreenshot, mojom::kNotifierScreenCapture,
    mojom::kNotifierScreenShare, mojom::kNotifierSessionLengthTimeout,
    mojom::kNotifierSms, mojom::kNotifierSupervisedUser, mojom::kNotifierTether,
    mojom::kNotifierWebUsb, mojom::kNotifierWifiToggle,
    // Note: Order doesn't matter here, so keep this in alphabetic order, don't
    // just add your stuff at the end!
    NULL};

bool MatchSystemNotifierId(const message_center::NotifierId& notifier_id,
                           const char* id_list[]) {
  if (notifier_id.type != message_center::NotifierId::SYSTEM_COMPONENT)
    return false;

  for (size_t i = 0; id_list[i] != NULL; ++i) {
    if (notifier_id.id == id_list[i])
      return true;
  }
  return false;
}

}  // namespace

bool ShouldAlwaysShowPopups(const message_center::NotifierId& notifier_id) {
  return MatchSystemNotifierId(notifier_id, kAlwaysShownSystemNotifierIds);
}

bool IsAshSystemNotifier(const message_center::NotifierId& notifier_id) {
  return ShouldAlwaysShowPopups(notifier_id) ||
         MatchSystemNotifierId(notifier_id, kAshSystemNotifiers);
}

std::unique_ptr<message_center::Notification> CreateSystemNotification(
    message_center::NotificationType type,
    const std::string& id,
    const base::string16& title,
    const base::string16& message,
    const gfx::Image& icon,
    const base::string16& display_source,
    const GURL& origin_url,
    const message_center::NotifierId& notifier_id,
    const message_center::RichNotificationData& optional_fields,
    scoped_refptr<message_center::NotificationDelegate> delegate,
    const gfx::VectorIcon& small_image,
    message_center::SystemNotificationWarningLevel color_type) {
  if (message_center::IsNewStyleNotificationEnabled()) {
    return message_center::Notification::CreateSystemNotification(
        type, id, title, message, gfx::Image(), display_source, origin_url,
        notifier_id, optional_fields, delegate, small_image, color_type);
  }
  return base::MakeUnique<message_center::Notification>(
      type, id, title, message, icon, display_source, origin_url, notifier_id,
      optional_fields, delegate);
}

}  // namespace system_notifier
}  // namespace ash
