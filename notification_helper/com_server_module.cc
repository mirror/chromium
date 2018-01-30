// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This macro is used in <wrl/module.h>. Since only the COM functionality is
// used here (while WinRT isn't being used), define this macro to optimize
// compilation of <wrl/module.h> for COM-only.
#ifndef __WRL_CLASSIC_COM_STRICT__
#define __WRL_CLASSIC_COM_STRICT__

#include "notification_helper/com_server_module.h"

#include <atltrace.h>
#include <wrl/module.h>

#include "chrome/install_static/install_util.h"
#include "notification_helper/notification_activator.h"

namespace notification_helper {

ComServerModule::ComServerModule()
    : com_object_released_(base::WaitableEvent::ResetPolicy::MANUAL,
                           base::WaitableEvent::InitialState::NOT_SIGNALED) {}

ComServerModule::~ComServerModule() = default;

HRESULT ComServerModule::Run() {
  HRESULT hr = RegisterClassObjects();
  if (FAILED(hr))
    return hr;

  WaitForAllObjectsReleased();
  UnregisterClassObjects();

  return hr;
}

HRESULT ComServerModule::RegisterClassObjects() {
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

  // Create an out-of-proc COM module with caching disabled. The supplied
  // callback (i.e., SignalObjectReleaseEvent) is called when the last instance
  // object of the module is released.
  auto& module =
      Microsoft::WRL::Module<Microsoft::WRL::OutOfProcDisableCaching>::Create(
          this, &ComServerModule::SignalObjectReleaseEvent);

  // Usually COM module classes statically define their CLSID at compile time
  // through the use of various macros, and WRL::Module internals takes care of
  // creating the class objects and registering them. However, we need to
  // register the same object with different CLSIDs depending on a runtime
  // setting, so we handle that logic here.
  IID class_ids[] = {install_static::GetToastActivatorClsid()};
  static_assert(arraysize(cookies_) == arraysize(class_ids),
                "Arrays cookies_ and class_ids must be the same size.");

  // The use of class_factory.GetAddressOf() below assumes that there's only one
  // factory being registered, which is true for now. We need to put the
  // interface pointer in an array like class_ids and cookies_ if more than one
  // class is ever registered.
  hr =
      module.RegisterCOMObject(nullptr, class_ids, class_factory.GetAddressOf(),
                               cookies_, arraysize(cookies_));
  if (FAILED(hr)) {
    ATL::AtlTrace("NotificationActivator registration failed\n");
    return hr;
  }

  return hr;
}

void ComServerModule::UnregisterClassObjects() {
  auto& module = Microsoft::WRL::Module<
      Microsoft::WRL::OutOfProcDisableCaching>::GetModule();
  HRESULT hr =
      module.UnregisterCOMObject(nullptr, cookies_, arraysize(cookies_));
  if (FAILED(hr))
    ATL::AtlTrace("NotificationActivator unregistration failed\n");
}

void ComServerModule::WaitForAllObjectsReleased() {
  com_object_released_.Wait();
}

void ComServerModule::SignalObjectReleaseEvent() {
  com_object_released_.Signal();
}

}  // namespace notification_helper

#endif  // __WRL_CLASSIC_COM_STRICT__
