// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NOTIFICATION_HELPER_COM_SERVER_MODULE_H
#define NOTIFICATION_HELPER_COM_SERVER_MODULE_H

#include "base/synchronization/waitable_event.h"
#include "base/win/windows_types.h"

// This class is used to keep track of registering and unregistering the
// NotificationActivator COM object.
class ComServerModule {
 public:
  ComServerModule();
  ~ComServerModule();

  // Handles object registration and unregistration. Returns when all registered
  // objects are released.
  HRESULT Run();

 private:
  // Registers the NotificationActivator COM object so other applications can
  // connect to it. Returns the registration status.
  HRESULT RegisterClassObjects();

  // Unregisters the NotificationActivator COM object.
  void UnregisterClassObjects();

  // Waits for all instance objects are released from the module.
  void WaitForAllObjectsReleased();

  // Signals the COM object release event.
  void SignalObjectReleaseEvent();

  // Identifies the class objects that were registered or to be unregistered.
  DWORD cookies_[1] = {0};

  // This starts "unsignaled", and is signaled when the last instance object is
  // released from the module.
  base::WaitableEvent com_object_released_;
};

#endif  // NOTIFICATION_HELPER_COM_SERVER_MODULE_H
