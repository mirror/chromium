// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_SERVER_NOTIFICATION_ACTIVATOR_H_
#define CHROME_SERVER_NOTIFICATION_ACTIVATOR_H_

#include "windows.h"

#include <NotificationActivationCallback.h>
#include <atltrace.h>
#include <wrl.h>

#include "chrome/install_static/install_util.h"

namespace mswr = Microsoft::WRL;

using Microsoft::WRL::ComPtr;

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
  ActivatorRegisterer() = default;

  ~ActivatorRegisterer() {
    // Unregister the NotificationActivator class object if it is now
    // registered.
    if (!has_registered_)
      return;
    mswr::Module<mswr::OutOfProcDisableCaching>& module =
        mswr::Module<mswr::OutOfProcDisableCaching>::GetModule();
    HRESULT hr = module.UnregisterCOMObject(nullptr, &cookies_, 1);
    if (FAILED(hr))
      ATL::AtlTrace("NotificationActivator unregistration failed\n");

    has_registered_ = false;
  }

  // Registers the NotificationActivator COM object so other applications can
  // connect to it.
  HRESULT Run() {
    ::MessageBoxW(NULL, L"enter Run", L"", MB_OK);
    // Usually COM module classes statically define their CLSID at compile time
    // through the use of various macros, and Windows internals takes care of
    // creating the class objects and registering them.  However, we need to
    // register the same object with different CLSIDs depending on a runtime
    // setting, so we handle that logic here.
    ComPtr<IUnknown> factory;
    unsigned int flags = mswr::ModuleType::OutOfProcDisableCaching;
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
    if (FAILED(hr)) {
      ATL::AtlTrace("Factory creation failed\n");
      return hr;
    }

    IID class_ids[] = {*entry.activationId.clsid};
    ComPtr<IClassFactory> class_factory;
    hr = factory.As(&class_factory);
    if (FAILED(hr))
      return hr;

    // Create a Out-of-Process COM module and register the NotificationActivator
    // class object to it.
    auto& module = mswr::Module<mswr::OutOfProcDisableCaching>::GetModule();

    hr = module.RegisterCOMObject(nullptr, class_ids,
                                  class_factory.GetAddressOf(), &cookies_, 1);
    if (FAILED(hr)) {
      ATL::AtlTrace("NotificationActivator registration failed\n");
      return hr;
    }

    has_registered_ = true;
    ::MessageBoxW(NULL, L"exit Run", L"", MB_OK);
    return hr;
  }

 private:
  // A boolean flag indicating if the NotificationActivator class object has
  // registered or not.
  bool has_registered_ = false;

  // Identifies the class objects that were registered or to be unregistered.
  DWORD cookies_ = 0;
};
#endif  // CHROME_SERVER_NOTIFICATION_ACTIVATOR_H_
