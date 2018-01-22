// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_SERVER_NOTIFICATION_ACTIVATOR_H_
#define CHROME_SERVER_NOTIFICATION_ACTIVATOR_H_

#include "windows.h"

#include <NotificationActivationCallback.h>
#include <wrl.h>

#include "base/logging.h"
#include "chrome/install_static/install_util.h"

namespace mswr = Microsoft::WRL;

// A Win32 component that participates with Action Center will need to create a
// COM component that exposes the INotificationActivationCallback interface.
class NotificationActivator
    : public mswr::RuntimeClass<mswr::RuntimeClassFlags<mswr::ClassicCom>,
                                INotificationActivationCallback> {
 public:
  IFACEMETHODIMP Activate(LPCWSTR /* appUserModelId */,
                          LPCWSTR /* invokedArgs */,
                          const NOTIFICATION_USER_INPUT_DATA* /* data */,
                          ULONG /* count */) override;

 public:
  NotificationActivator();
  ~NotificationActivator() override;
};

// Keeps track of registering and unregistering the NotificationActivator
// COM object.
class ActivatorRegisterer {
 public:
  ActivatorRegisterer() = default;

  ~ActivatorRegisterer() {
    // Unregister the NotificationActivator class object if it is now
    // registered.
    if (!has_registered_)
      return;
    mswr::Module<mswr::OutOfProc>& module =
        mswr::Module<mswr::OutOfProc>::GetModule();
    HRESULT hr = module.UnregisterCOMObject(nullptr, &cookies_, 1);
    if (FAILED(hr))
      LOG(ERROR) << "NotificationActivator unregistration failed " << hr;

    has_registered_ = false;
  }

  // Registers the NotificationActivator COM object so other applications can
  // connect to it.
  HRESULT Run() {
    // Usually COM module classes statically define their CLSID at compile time
    // through the use of various macros, and Windows internals takes care of
    // creating the class objects and registering them.  However, we need to
    // register the same object with different CLSIDs depending on a runtime
    // setting, so we handle that logic here.
    IUnknown* factory = nullptr;
    unsigned int flags = mswr::ModuleType::OutOfProc;
    mswr::Details::FactoryCache factory_cache;
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
      LOG(ERROR) << "Factory creation failed " << hr;
      return hr;
    }

    IID class_ids[] = {*entry.activationId.clsid};
    IClassFactory* class_factories[] = {
        reinterpret_cast<IClassFactory*>(factory)};

    // Create a Out-of-Process COM module and register the NotificationActivator
    // class object to it.
    mswr::Module<mswr::OutOfProc>::Create([] {});
    mswr::Module<mswr::OutOfProc>& module =
        mswr::Module<mswr::OutOfProc>::GetModule();

    hr = module.RegisterCOMObject(nullptr, class_ids, class_factories,
                                  &cookies_, 1);
    if (FAILED(hr)) {
      LOG(ERROR) << "NotificationActivator registration failed " << hr;
      return hr;
    }

    has_registered_ = true;
    return hr;
  }

 private:
  // A boolean indicating if the NotificationActivator class object has
  // registered or not.
  bool has_registered_ = false;

  // An array of pointers to values that identify the class objects that were
  // registered or to be unregistered.
  DWORD cookies_;
};

#endif  // CHROME_SERVER_NOTIFICATION_ACTIVATOR_H_
