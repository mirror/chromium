// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_SERVER_NOTIFICATION_ACTIVATOR_H_
#define CHROME_SERVER_NOTIFICATION_ACTIVATOR_H_

// This macro is used in wrl/module.h. Since only the COM functionality is
// used here (while WinRT isn't being used), define this macro to optimize
// wrl/module.h for COM-only.
#ifndef __WRL_CLASSIC_COM_STRICT__
#define __WRL_CLASSIC_COM_STRICT__

#include "windows.h"

#include <NotificationActivationCallback.h>
#include <wrl.h>

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

  IFACEMETHODIMP Activate(LPCWSTR /* appUserModelId */,
                          LPCWSTR /* invokedArgs */,
                          const NOTIFICATION_USER_INPUT_DATA* /* data */,
                          ULONG /* count */) override;
};

// This class is used to keep track of registering and unregistering the
// NotificationActivator COM object.
class ActivatorRegisterer {
 public:
  ActivatorRegisterer();
  ~ActivatorRegisterer();

  // Registers the NotificationActivator COM object so other applications can
  // connect to it.
  HRESULT Run();

 private:
  // Signals the COM object release event.
  void SignalObjectReleaseEvent();

  // A boolean flag indicating if the NotificationActivator class object has
  // registered or not.
  bool has_registered_ = false;

  // Identifies the class objects that were registered or to be unregistered.
  DWORD cookies_ = 0;

  // This starts "unsignaled", and is signaled when the last instance object is
  // released from the module.
  base::WaitableEvent com_object_released_;
};

#endif  // __WRL_CLASSIC_COM_STRICT__

#endif  // CHROME_SERVER_NOTIFICATION_ACTIVATOR_H_
