// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "notification_helper/notification_activator.h"

NotificationActivator::~NotificationActivator() = default;

IFACEMETHODIMP NotificationActivator::Activate(
    LPCWSTR appUserModelId,
    LPCWSTR invokedArgs,
    const NOTIFICATION_USER_INPUT_DATA* data,
    ULONG count) {
  ::MessageBoxW(NULL, L"Activate", L"", MB_OK);
  // When Chrome is running, NotificationPlatformBridgeWinImpl::OnActivated will
  // be triggered after this function call returns.

  // TODO(chengx): Investigate the correct activate behavior (mainly when Chrome
  // is not running) and implement it. For example, decode the useful data from
  // the function input and launch Chrome with proper args.

  // An example of input parameter value. The toast from which the activation
  // comes is generated from https://tests.peter.sh/notification-generator/ with
  // the default setting.
  // {
  //    appUserModelId = "Chromium.KX56J2SJSCJTWGPH2RNH3MHAM4"
  //    invokedArgs =
  //         "0|Default|0|https://tests.peter.sh/|p#https://tests.peter.sh/#01"
  //    data = nullptr
  //    count = 0
  // }

  return S_OK;
}
