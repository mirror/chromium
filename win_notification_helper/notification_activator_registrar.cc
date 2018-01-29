// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This macro is used in <wrl/module.h>. Since only the COM functionality is
// used here (while WinRT isn't being used), define this macro to optimize
// compilation of <wrl/module.h> for COM-only.
#ifndef __WRL_CLASSIC_COM_STRICT__
#define __WRL_CLASSIC_COM_STRICT__

#include "win_notification_helper/notification_activator_registrar.h"

#include <atltrace.h>
#include <wrl/module.h>

#include "chrome/install_static/install_util.h"
#include "win_notification_helper/notification_activator.h"

NotificationActivatorRegistrar::NotificationActivatorRegistrar()
    : com_object_released_(base::WaitableEvent::ResetPolicy::MANUAL,
                           base::WaitableEvent::InitialState::NOT_SIGNALED) {}

NotificationActivatorRegistrar::~NotificationActivatorRegistrar() {}

HRESULT NotificationActivatorRegistrar::Run() {
  HRESULT hr = RegisterClassObjects();
  if (FAILED(hr))
    return hr;

  WaitForAllObjectsReleased();
  return UnregisterClassObjects();
}

HRESULT NotificationActivatorRegistrar::RegisterClassObjects() {
  // Create the class factory for NotificationActivator.
  Microsoft::WRL::ComPtr<IUnknown> factory;
  unsigned int flags = Microsoft::WRL::ModuleType::OutOfProcDisableCaching;

  HRESULT hr = ::Microsoft::WRL::Details::CreateClassFactory<
      ::Microsoft::WRL::SimpleClassFactory<NotificationActivator>>(
      &flags, nullptr, __uuidof(IClassFactory), &factory);

  if (FAILED(hr)) {
    ATL::AtlTrace("Factory creation failed\n");
    return hr;
  }

  Microsoft::WRL::ComPtr<IClassFactory> class_factory;
  hr = factory.As(&class_factory);

  if (FAILED(hr))
    return hr;

  // Create a Out-of-Process COM module with caching disabled. The supplied
  // callback (i.e., SignalObjectReleaseEvent) is called when the last
  // instance object of the module is released.
  auto& module =
      Microsoft::WRL::Module<Microsoft::WRL::OutOfProcDisableCaching>::Create(
          this, &NotificationActivatorRegistrar::SignalObjectReleaseEvent);

  // Usually COM module classes statically define their CLSID at compile time
  // through the use of various macros, and Windows internals takes care of
  // creating the class objects and registering them. However, we need to
  // register the same object with different CLSIDs depending on a runtime
  // setting, so we handle that logic here.
  IID class_ids[] = {install_static::GetToastActivatorClsid()};
  static_assert(arraysize(cookies_) == arraysize(class_ids),
                "Arrays cookies_ and class_ids must have a same size.");
  hr =
      module.RegisterCOMObject(nullptr, class_ids, class_factory.GetAddressOf(),
                               cookies_, arraysize(cookies_));
  if (FAILED(hr)) {
    ATL::AtlTrace("NotificationActivator registration failed\n");
    return hr;
  }

  return hr;
}

HRESULT NotificationActivatorRegistrar::UnregisterClassObjects() {
  auto& module = Microsoft::WRL::Module<
      Microsoft::WRL::OutOfProcDisableCaching>::GetModule();
  HRESULT hr =
      module.UnregisterCOMObject(nullptr, cookies_, arraysize(cookies_));
  if (FAILED(hr))
    ATL::AtlTrace("NotificationActivator unregistration failed\n");

  return hr;
}

void NotificationActivatorRegistrar::WaitForAllObjectsReleased() {
  com_object_released_.Wait();
}

void NotificationActivatorRegistrar::SignalObjectReleaseEvent() {
  com_object_released_.Signal();
}

#endif  // __WRL_CLASSIC_COM_STRICT__
