// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome_server/notification_activator.h"

#include <atltrace.h>

#include "chrome/install_static/install_util.h"
#include "chrome_server/chrome_util.h"

NotificationActivator::NotificationActivator() {}

NotificationActivator::~NotificationActivator() {}

IFACEMETHODIMP NotificationActivator::Activate(
    LPCWSTR /* appUserModelId */,
    LPCWSTR /* invokedArgs */,
    const NOTIFICATION_USER_INPUT_DATA* /* data */,
    ULONG /* count */) {

  // TODO(chengx): Investigate the correct behavior here and implement it.
  chrome_server::LaunchChromeIfNotRunning();

  return S_OK;
}

ActivatorRegisterer::ActivatorRegisterer()
    : com_object_released_(base::WaitableEvent::ResetPolicy::MANUAL,
                           base::WaitableEvent::InitialState::NOT_SIGNALED) {}

ActivatorRegisterer::~ActivatorRegisterer() {
  // Unregister the NotificationActivator class object if it is now
  // registered.
  if (!has_registered_)
    return;

  auto& module = mswr::Module<mswr::OutOfProcDisableCaching>::GetModule();

  HRESULT hr = module.UnregisterCOMObject(nullptr, &cookies_, 1);
  if (FAILED(hr))
    ATL::AtlTrace("NotificationActivator unregistration failed\n");

  has_registered_ = false;
}

// Registers the NotificationActivator COM object so other applications can
// connect to it.
HRESULT ActivatorRegisterer::Run() {
  // Usually COM module classes statically define their CLSID at compile time
  // through the use of various macros, and Windows internals takes care of
  // creating the class objects and registering them.  However, we need to
  // register the same object with different CLSIDs depending on a runtime
  // setting, so we handle that logic here.
  Microsoft::WRL::ComPtr<IUnknown> factory;
  unsigned int flags = mswr::ModuleType::OutOfProcDisableCaching;

  // ----
  mswr::Details::FactoryCache factory_cache = {};
  mswr::Details::CreatorMap entry = {
      ::mswr::Details::CreateClassFactory<
          ::mswr::SimpleClassFactory<NotificationActivator>>,
      {&install_static::GetToastActivatorClsid()},
      nullptr,
      &factory_cache,
      nullptr};

  HRESULT hr =
      entry.factoryCreator(&flags, &entry, __uuidof(IClassFactory), &factory);

  // HRESULT hr = ::mswr::Details::CreateClassFactory<
  //    ::mswr::SimpleClassFactory<NotificationActivator>>(
  //    &flags, nullptr, __uuidof(IClassFactory), &factory);
  // ----

  if (FAILED(hr)) {
    ATL::AtlTrace("Factory creation failed\n");
    return hr;
  }

  // ---
  IID class_ids[] = {*entry.activationId.clsid};
  /*IID class_ids[] = {install_static::GetToastActivatorClsid()};*/
  // ---
  Microsoft::WRL::ComPtr<IClassFactory> class_factory;
  hr = factory.As(&class_factory);
  if (FAILED(hr))
    return hr;

  // Create a Out-of-Process COM module with caching disabled. The supplied
  // callback (i.e., SignalObjectReleaseEvent) is called when the last
  // instance object of the module is released.
  mswr::Module<mswr::OutOfProcDisableCaching>::Create<>(
      [this]() { this->SignalObjectReleaseEvent(); });

  // Register the NotificationActivator class object.
  auto& module = mswr::Module<mswr::OutOfProcDisableCaching>::GetModule();

  hr = module.RegisterCOMObject(nullptr, class_ids,
                                class_factory.GetAddressOf(), &cookies_, 1);
  if (FAILED(hr)) {
    ATL::AtlTrace("NotificationActivator registration failed\n");
    return hr;
  }

  has_registered_ = true;

  com_object_released_.Wait();
  return hr;
}

void ActivatorRegisterer::SignalObjectReleaseEvent() {
  com_object_released_.Signal();
}
