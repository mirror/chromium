// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "win_notification_helper/notification_activator.h"

#include <atltrace.h>

#include "chrome/install_static/install_util.h"

NotificationActivator::NotificationActivator() {}

NotificationActivator::~NotificationActivator() {}

IFACEMETHODIMP NotificationActivator::Activate(
    LPCWSTR appUserModelId,
    LPCWSTR invokedArgs,
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
  //    appUserModelId = "Chromium.KX56J2SJSCJTWGPH2RNH3MHAM4"
  //    invokedArgs =
  //         "0|Default|0|https://tests.peter.sh/|p#https://tests.peter.sh/#01"
  //    data = nullptr
  //    count = 0
  // }

  return S_OK;
}

ActivatorRegisterer::ActivatorRegisterer()
    : com_object_released_(base::WaitableEvent::ResetPolicy::MANUAL,
                           base::WaitableEvent::InitialState::NOT_SIGNALED) {}

ActivatorRegisterer::~ActivatorRegisterer() {}

HRESULT ActivatorRegisterer::Run() {
  HRESULT hr = RegisterClassObjects();
  if (FAILED(hr))
    return hr;

  WaitForAllObjectsReleased();
  UnregisterClassObjects();
  return hr;
}

HRESULT ActivatorRegisterer::RegisterClassObjects() {
  // Create the class factory for NotificationActivator.
  Microsoft::WRL::ComPtr<IUnknown> factory;
  unsigned int flags = mswr::ModuleType::OutOfProcDisableCaching;

  HRESULT hr = ::mswr::Details::CreateClassFactory<
      ::mswr::SimpleClassFactory<NotificationActivator>>(
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
  auto& module = mswr::Module<mswr::OutOfProcDisableCaching>::Create(
      this, &ActivatorRegisterer::SignalObjectReleaseEvent);

  // Usually COM module classes statically define their CLSID at compile time
  // through the use of various macros, and Windows internals takes care of
  // creating the class objects and registering them.  However, we need to
  // register the same object with different CLSIDs depending on a runtime
  // setting, so we handle that logic here.
  IID class_ids[] = {install_static::GetToastActivatorClsid()};
  static_assert(arraysize(cookies_) == arraysize(class_ids),
                "These two arrays must have a same size.");
  hr =
      module.RegisterCOMObject(nullptr, class_ids, class_factory.GetAddressOf(),
                               cookies_, arraysize(cookies_));
  if (FAILED(hr)) {
    ATL::AtlTrace("NotificationActivator registration failed\n");
    return hr;
  }

  return hr;
}

void ActivatorRegisterer::WaitForAllObjectsReleased() {
  com_object_released_.Wait();
}

HRESULT ActivatorRegisterer::UnregisterClassObjects() {
  auto& module = mswr::Module<mswr::OutOfProcDisableCaching>::GetModule();
  HRESULT hr =
      module.UnregisterCOMObject(nullptr, cookies_, arraysize(cookies_));
  if (FAILED(hr))
    ATL::AtlTrace("NotificationActivator unregistration failed\n");
  return hr;
}

void ActivatorRegisterer::SignalObjectReleaseEvent() {
  com_object_released_.Signal();
}
