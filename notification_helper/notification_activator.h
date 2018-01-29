// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NOTIFICATION_HELPER_NOTIFICATION_ACTIVATOR_H_
#define NOTIFICATION_HELPER_NOTIFICATION_ACTIVATOR_H_

#include <windows.h>

#include <NotificationActivationCallback.h>
#include <wrl/implements.h>

// A Win32 component that participates with Action Center will need to create a
// COM component that exposes the INotificationActivationCallback interface.
class NotificationActivator
    : public Microsoft::WRL::RuntimeClass<
          Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
          INotificationActivationCallback> {
 public:
  NotificationActivator() = default;
  ~NotificationActivator() override;

  // Called when a user interacts with a toast in the Windows action center.
  // For the meaning of the input parameters, see
  // https://msdn.microsoft.com/library/windows/desktop/mt643712.aspx
  IFACEMETHODIMP Activate(LPCWSTR appUserModelId,
                          LPCWSTR invokedArgs,
                          const NOTIFICATION_USER_INPUT_DATA* data,
                          ULONG count) override;
};

#endif  // NOTIFICATION_HELPER_NOTIFICATION_ACTIVATOR_H_
