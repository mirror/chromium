// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome_server/notification_activator.h"

#include "chrome_server/chrome_util.h"

NotificationActivator::NotificationActivator() {}

NotificationActivator::~NotificationActivator() {}

IFACEMETHODIMP NotificationActivator::Activate(
    LPCWSTR /* appUserModelId */,
    LPCWSTR /* invokedArgs */,
    const NOTIFICATION_USER_INPUT_DATA* /* data */,
    ULONG /* count */) {
  // TODO(chengx): Investigate the correct behavior here and implement it.
  chrome_server::LaunchChromeIfNotRunning();

  return S_OK;
}
