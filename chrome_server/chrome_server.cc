// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include <windows.h>

#include <atlbase.h>
#include <atlcom.h>
#include <atlctl.h>
#include <initguid.h>
#include <shellapi.h>
#include <wrl.h>
#include <wrl/client.h>

#include "base/at_exit.h"
#include "base/strings/string16.h"
#include "base/win/scoped_winrt_initializer.h"
#include "chrome/install_static/install_util.h"
#include "chrome/install_static/product_install_details.h"
#include "chrome_server/notification_activator.h"
#include "chrome_server/resource.h"

using namespace ATL;
namespace mswr = Microsoft::WRL;

// Usually classes derived from CAtlExeModuleT, or other types of ATL
// COM module classes statically define their CLSID at compile time through
// the use of various macros, and ATL internals takes care of creating the
// class objects and registering them.  However, we need to register the same
// object with different CLSIDs depending on a runtime setting, so we handle
// that logic here, before the main ATL message loop runs.
class NotificationActivatorModule
    : public ATL::CAtlExeModuleT<NotificationActivatorModule> {
 public:
  typedef ATL::CAtlExeModuleT<NotificationActivatorModule> ParentClass;
  typedef CComObject<NotificationActivator> ImplType;

  NotificationActivatorModule() : registration_token_(0) {}

  HRESULT PreMessageLoop(int nShowCmd) {
    HRESULT hr = S_OK;
    GUID clsid = install_static::GetToastActivatorClsid();

    // We use the same class creation logic as ATL itself.  See
    // _ATL_OBJMAP_ENTRY::RegisterClassObject() in atlbase.h
    hr = ImplType::_ClassFactoryCreatorClass::CreateInstance(
        ImplType::_CreatorClass::CreateInstance, IID_PPV_ARGS(&instance_));

    if (FAILED(hr))
      return hr;
    hr = ::CoRegisterClassObject(clsid, instance_.Get(), CLSCTX_LOCAL_SERVER,
                                 REGCLS_MULTIPLEUSE | REGCLS_SUSPENDED,
                                 &registration_token_);
    if (FAILED(hr))
      return hr;

    /*base::win::ScopedWinrtInitializer scoped_winrt_initializer;
    if (!scoped_winrt_initializer.Succeeded()) {
      LOG(ERROR) << "Failed initializing win RT.";
      return S_FALSE;
    }

    mswr::Module<mswr::OutOfProc>::Create();
    mswr::Module<mswr::OutOfProc>& module =
        mswr::Module<mswr::OutOfProc>::GetModule();

    Microsoft::WRL::ComPtr<IUnknown> factory;
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
    Microsoft::WRL::ComPtr<IClassFactory> class_factory;
    hr = factory.As(&class_factory);
    if (FAILED(hr))
      return hr;

    DWORD cookies_;
    hr = module.RegisterCOMObject(nullptr, class_ids,
                                  class_factory.GetAddressOf(), &cookies_, 1);

    if (FAILED(hr)) {
      LOG(ERROR) << "NotificationActivator registration failed " << hr;
      return hr;
    }*/

    return ParentClass::PreMessageLoop(nShowCmd);
  }

  HRESULT PostMessageLoop() {
    if (registration_token_ != 0) {
      ::CoRevokeClassObject(registration_token_);
      registration_token_ = 0;
    }

    instance_.Reset();

    return ParentClass::PostMessageLoop();
  }

 private:
  Microsoft::WRL::ComPtr<IUnknown> instance_;
  DWORD registration_token_;
};

NotificationActivatorModule _AtlModule;

extern "C" int WINAPI wWinMain(HINSTANCE /* instance */,
                               HINSTANCE /* previous_instance */,
                               LPWSTR /* command_line */,
                               int nShowCmd) {
  install_static::InitializeProductDetailsForPrimaryModule();

  base::AtExitManager exit_manager;
  AtlTrace("chrome_server enter\n");

  HRESULT ret_code = _AtlModule.WinMain(nShowCmd);

  AtlTrace("chrome_server exit, code = %d\n", ret_code);
  return ret_code;
}
