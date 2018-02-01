// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This macro is used in <wrl/module.h>. Since only the COM functionality is
// used here (while WinRT isn't being used), define this macro to optimize
// compilation of <wrl/module.h> for COM-only.
#ifndef __WRL_CLASSIC_COM_STRICT__
#define __WRL_CLASSIC_COM_STRICT__
#endif  // __WRL_CLASSIC_COM_STRICT__

#include "notification_helper/com_server_module.h"

#include <wrl/module.h>

#include "chrome/install_static/install_util.h"
#include "notification_helper/notification_activator.h"
#include "notification_helper/trace_util.h"

namespace notification_helper {

namespace {

// This value is used to initialize the WaitableEvent object. This MUST BE set
// to MANUAL for correct operation of the IsSignaled() call in
// IsEventSignaled(). See the comment there for why.
constexpr base::WaitableEvent::ResetPolicy kResetPolicy =
    base::WaitableEvent::ResetPolicy::MANUAL;

}  // namespace

ComServerModule::ComServerModule()
    : com_object_released_(kResetPolicy,
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
  // Create an out-of-proc COM module with caching disabled. The supplied
  // method is invoked when the last instance object of the module is released.
  auto& module =
      Microsoft::WRL::Module<Microsoft::WRL::OutOfProcDisableCaching>::Create(
          this, &ComServerModule::SignalObjectReleaseEvent);

  // Create the class factory for NotificationActivator.
  IUnknown* factory = nullptr;
  unsigned int flags = Microsoft::WRL::ModuleType::OutOfProcDisableCaching;

  HRESULT hr = ::Microsoft::WRL::Details::CreateClassFactory<
      ::Microsoft::WRL::SimpleClassFactory<NotificationActivator>>(
      &flags, nullptr, __uuidof(IClassFactory), &factory);
  if (FAILED(hr)) {
    Trace(L"Factory creation failed\n");
    return hr;
  }

  IClassFactory* class_factories[] = {
      reinterpret_cast<IClassFactory*>(factory)};

  // Usually COM module classes statically define their CLSID at compile time
  // through the use of various macros, and WRL::Module internals takes care of
  // creating the class objects and registering them. However, we need to
  // register the same object with different CLSIDs depending on a runtime
  // setting, so we handle that logic here.
  IID class_ids[] = {install_static::GetToastActivatorClsid()};
  static_assert(arraysize(cookies_) == arraysize(class_ids),
                "Arrays cookies_ and class_ids must be the same size.");

  hr = module.RegisterCOMObject(nullptr, class_ids, class_factories, cookies_,
                                arraysize(cookies_));
  if (FAILED(hr)) {
    Trace(L"NotificationActivator registration failed\n");
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
    Trace(L"NotificationActivator unregistration failed\n");
}

bool ComServerModule::IsEventSignaled() {
  // The IsSignaled() check below requires that the WaitableEvent be manually
  // reset, to avoid signaling the event in IsSignaled() itself.
  static_assert(kResetPolicy == base::WaitableEvent::ResetPolicy::MANUAL,
                "The reset policy must be set to MANUAL");
  return com_object_released_.IsSignaled();
}

void ComServerModule::WaitForAllObjectsReleased() {
  com_object_released_.Wait();
}

void ComServerModule::SignalObjectReleaseEvent() {
  com_object_released_.Signal();
}

}  // namespace notification_helper
