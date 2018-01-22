// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome_server/notification_activator.h"

#include <algorithm>

#include "chrome_server/chrome_util.h"

NotificationActivator::NotificationActivator() {
  /*::MessageBoxW(NULL, L"NotificationActivator constructor", L"", MB_OK);*/
}

NotificationActivator::~NotificationActivator() {
  ::MessageBoxW(NULL, L"NotificationActivator destructor", L"", MB_OK);
}

IFACEMETHODIMP NotificationActivator::Activate(
    LPCWSTR /* appUserModelId */,
    LPCWSTR /* invokedArgs */,
    const NOTIFICATION_USER_INPUT_DATA* /* data */,
    ULONG /* count */) {
  ::MessageBoxW(NULL, L"NotificationActivator::Activate", L"", MB_OK);

  chrome_server::LaunchChromeIfNotRunning();

  return S_OK;
}
