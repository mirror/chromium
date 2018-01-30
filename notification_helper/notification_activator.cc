// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "notification_helper/notification_activator.h"

NotificationActivator::~NotificationActivator() = default;

IFACEMETHODIMP NotificationActivator::Activate(
    LPCWSTR app_user_model_id,
    LPCWSTR invoked_args,
    const NOTIFICATION_USER_INPUT_DATA* data,
    ULONG count) {
  // When Chrome is running, NotificationPlatformBridgeWinImpl::OnActivated will
  // be triggered after this function call returns.

  // TODO(chengx): Investigate the correct activate behavior (mainly when Chrome
  // is not running) and implement it. For example, decode the useful data from
  // the function input and launch Chrome with proper args.

  // An example of input parameter value. The toast from which the activation
  // comes is generated from https://tests.peter.sh/notification-generator/ with
  // the default setting.
  // {
  //    app_user_model_id = "Chromium.KX56J2SJSCJTWGPH2RNH3MHAM4"
  //    invoked_args =
  //         "0|Default|0|https://tests.peter.sh/|p#https://tests.peter.sh/#01"
  //    data = nullptr
  //    count = 0
  // }

  // |invoked_args| has the same string value as |toast_id| defined in
  // HandleEvent() in notification_platform_bridge_win.cc, which is called by
  // NotificationPlatformBridgeWinImpl::OnActivated.
  //
  // |toast_id| (with the example value of |invoked_args| above) will be decoded
  // into
  //    |notification_type| = 0
  //           |profile_id| = Default
  //            |incognito| = 0
  //           |origin_url| = https://tests.peter.sh/
  //      |notification_id| = p#https://tests.peter.sh/#01
  //
  // This is basically all the information we need to handle an event in Chrome.

  return S_OK;
}
