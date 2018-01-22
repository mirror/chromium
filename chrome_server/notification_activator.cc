// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome_server/notification_activator.h"

#include "chrome_server/chrome_util.h"

NotificationActivator::NotificationActivator() {}

NotificationActivator::~NotificationActivator() {
  // TODO(chengx): remove, for test purpose.
  ::MessageBoxW(NULL, L"NotificationActivator destructor", L"", MB_OK);
}

IFACEMETHODIMP NotificationActivator::Activate(
    LPCWSTR /* appUserModelId */,
    LPCWSTR /* invokedArgs */,
    const NOTIFICATION_USER_INPUT_DATA* /* data */,
    ULONG /* count */) {
  // TODO(chengx): remove, for test purpose.
  ::MessageBoxW(NULL, L"NotificationActivator::Activate", L"", MB_OK);

  chrome_server::LaunchChromeIfNotRunning();

  return S_OK;
}
