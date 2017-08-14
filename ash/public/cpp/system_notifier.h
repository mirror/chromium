// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_PUBLIC_CPP_SYSTEM_NOTIFIER_H_
#define ASH_PUBLIC_CPP_SYSTEM_NOTIFIER_H_

#include <string>

#include "ash/public/cpp/ash_public_export.h"
#include "ui/message_center/notifier_settings.h"

namespace ash {
namespace system_notifier {

// The list of ash system notifier IDs. Alphabetical order.
ASH_PUBLIC_EXPORT extern const char kNotifierAccessibility[];
ASH_PUBLIC_EXPORT extern const char kNotifierBattery[];
ASH_PUBLIC_EXPORT extern const char kNotifierBluetooth[];
ASH_PUBLIC_EXPORT extern const char kNotifierCapsLock[];
ASH_PUBLIC_EXPORT extern const char kNotifierDeprecatedAccelerator[];
ASH_PUBLIC_EXPORT extern const char kNotifierDisk[];
ASH_PUBLIC_EXPORT extern const char kNotifierDisplay[];
ASH_PUBLIC_EXPORT extern const char kNotifierDisplayResolutionChange[];
ASH_PUBLIC_EXPORT extern const char kNotifierDisplayError[];
ASH_PUBLIC_EXPORT extern const char kNotifierDualRole[];
ASH_PUBLIC_EXPORT extern const char kNotifierFingerprintUnlock[];
ASH_PUBLIC_EXPORT extern const char kNotifierHats[];
ASH_PUBLIC_EXPORT extern const char kNotifierLocale[];
ASH_PUBLIC_EXPORT extern const char kNotifierMultiProfileFirstRun[];
ASH_PUBLIC_EXPORT extern const char kNotifierNetwork[];
ASH_PUBLIC_EXPORT extern const char kNotifierNetworkError[];
ASH_PUBLIC_EXPORT extern const char kNotifierNetworkPortalDetector[];
ASH_PUBLIC_EXPORT extern const char kNotifierPinUnlock[];
ASH_PUBLIC_EXPORT extern const char kNotifierPower[];
ASH_PUBLIC_EXPORT extern const char kNotifierScreenshot[];
ASH_PUBLIC_EXPORT extern const char kNotifierScreenCapture[];
ASH_PUBLIC_EXPORT extern const char kNotifierScreenShare[];
ASH_PUBLIC_EXPORT extern const char kNotifierSessionLengthTimeout[];
ASH_PUBLIC_EXPORT extern const char kNotifierSms[];
ASH_PUBLIC_EXPORT extern const char kNotifierStylusBattery[];
ASH_PUBLIC_EXPORT extern const char kNotifierSupervisedUser[];
ASH_PUBLIC_EXPORT extern const char kNotifierTether[];
ASH_PUBLIC_EXPORT extern const char kNotifierWebUsb[];
ASH_PUBLIC_EXPORT extern const char kNotifierWifiToggle[];

// Returns true if notifications from |notifier_id| should always appear as
// popups. "Always appear" means the popups should appear even in login screen,
// lock screen, or fullscreen state.
ASH_PUBLIC_EXPORT bool ShouldAlwaysShowPopups(
    const message_center::NotifierId& notifier_id);

// Returns true if |notifier_id| is the system notifier from Ash.
ASH_PUBLIC_EXPORT bool IsAshSystemNotifier(
    const message_center::NotifierId& notifier_id);

}  // namespace system_notifier
}  // namespace ash

#endif  // ASH_PUBLIC_CPP_SYSTEM_NOTIFIER_H_
