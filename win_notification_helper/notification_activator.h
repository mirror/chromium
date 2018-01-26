// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WIN_NOTIFICATION_HELPER_NOTIFICATION_ACTIVATOR_H_
#define WIN_NOTIFICATION_HELPER_NOTIFICATION_ACTIVATOR_H_

// TODO(chengx): Investigate the correct way of defining
// __WRL_CLASSIC_COM_STRICT__ to optimize wrl\module.h without breaking
// anything.

#include "windows.h"

#include <NotificationActivationCallback.h>
#include <wrl\module.h>

#include "base/synchronization/waitable_event.h"

namespace mswr = Microsoft::WRL;

// A Win32 component that participates with Action Center will need to create a
// COM component that exposes the INotificationActivationCallback interface.
class NotificationActivator
    : public mswr::RuntimeClass<mswr::RuntimeClassFlags<mswr::ClassicCom>,
                                INotificationActivationCallback> {
 public:
  NotificationActivator();
  ~NotificationActivator() override;

  // Called when a user interacts with a toast in the Windows action center.
  // For the meaning of the input parameters, see
  // https://msdn.microsoft.com/en-us/library/windows/desktop/mt643712(v=vs.85).aspx
  IFACEMETHODIMP Activate(LPCWSTR appUserModelId,
                          LPCWSTR invokedArgs,
                          const NOTIFICATION_USER_INPUT_DATA* data,
                          ULONG count) override;
};

// This class is used to keep track of registering and unregistering the
// NotificationActivator COM object.
class ActivatorRegisterer {
 public:
  ActivatorRegisterer();
  ~ActivatorRegisterer();

  // Manages object registration by calling RegisterClassObjects,
  // WaitForAllObjectsReleased, UnregisterClassObjects in sequence.
  HRESULT Run();

 private:
  // Registers the NotificationActivator COM object so other applications can
  // connect to it. Returns the registration status.
  HRESULT RegisterClassObjects();

  // Waits for all instance objects are released from the module.
  void WaitForAllObjectsReleased();

  // Unregisters the NotificationActivator COM object. Returns the
  // unregistration status.
  HRESULT UnregisterClassObjects();

  // Signals the COM object release event.
  void SignalObjectReleaseEvent();

  // Identifies the class objects that were registered or to be unregistered.
  DWORD cookies_[1] = {0};

  // This starts "unsignaled", and is signaled when the last instance object is
  // released from the module.
  base::WaitableEvent com_object_released_;
};

#endif  // WIN_NOTIFICATION_HELPER_NOTIFICATION_ACTIVATOR_H_
