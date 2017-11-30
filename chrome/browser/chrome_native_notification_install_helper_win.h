// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROME_NATIVE_NOTIFICATION_INSTALL_HELPER_WIN_H_
#define CHROME_BROWSER_CHROME_NATIVE_NOTIFICATION_INSTALL_HELPER_WIN_H_

#include <wrl.h>

// TODO(chengx): Different id for different browser distribution.
// TODO(finnur): This turned out to be trickier than it looks, because this is
// used as a compile const in NotificationActivator but the channels are
// determined at runtime.
#define CLSID_KEY "E65AECC7-DD9B-4D14-A4ED-73A5BEF1187E"

bool InstallShortcutAndRegisterComService(const CLSID& clsid);

#endif  // CHROME_BROWSER_CHROME_NATIVE_NOTIFICATION_INSTALL_HELPER_WIN_H_
