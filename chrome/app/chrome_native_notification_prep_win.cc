// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/app/chrome_native_notification_prep_win.h"

#include <NotificationActivationCallback.h>
#include <wrl.h>

#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/registry.h"
#include "base/win/shortcut.h"
#include "chrome/installer/util/install_util.h"
#include "chrome/installer/util/shell_util.h"
#include "chrome/installer/util/util_constants.h"

namespace mswr = Microsoft::WRL;
namespace mswrd = Microsoft::WRL::Details;

// The three macros below are redefinitions of CoCreatableClass,
// CoCreatableClassWithFactory, and InternalWrlCreateCreatorMap in
// winrt/wrl/module.h
//
// The original definitions caused win_clang compile errors. To fix the errors,
// we removed __declspec(selectany) in InternalWrlCreateCreatorMap, and added
// braces to "0" and "runtimeClassName" in InternalWrlCreateCreatorMap.

#define CoCreatableClass2(className) \
  CoCreatableClassWithFactory2(className, mswr::SimpleClassFactory<className>)

#define CoCreatableClassWithFactory2(className, factory)                       \
  InternalWrlCreateCreatorMap2(className##_COM, &__uuidof(className), nullptr, \
                                                                               \
                               mswrd::CreateClassFactory<factory>,             \
                               "minATL$__f")

#define InternalWrlCreateCreatorMap2(className, runtimeClassName, trustLevel, \
                                                                              \
                                     creatorFunction, section)                \
  mswrd::FactoryCache __objectFactory__##className = {nullptr, {0}};          \
  extern const mswrd::CreatorMap __object_##className = {                     \
      creatorFunction,                                                        \
      {runtimeClassName},                                                     \
      trustLevel,                                                             \
      &__objectFactory__##className,                                          \
      nullptr};                                                               \
  extern "C" __declspec(allocate(section))                                    \
      const mswrd::CreatorMap* const __minATLObjMap_##className =             \
          &__object_##className;                                              \
  WrlCreatorMapIncludePragma(className)

// RoInitializeWrapper2 reimplements
// Microsoft::WRL::Wrappers::RoInitializeWrapper in wrl\wrappers\corewrappers.h
// with the LoadLibrary/GetProcAddress strategy.
//
// TODO(chengx): Move to core_winrt_util.h
class RoInitializeWrapper2 {
  HRESULT hr_ = S_OK;

 public:
  explicit RoInitializeWrapper2(RO_INIT_TYPE flags) {
    static HMODULE const handle = ::LoadLibrary(L"combase.dll");
    if (!handle)
      return;

    static const auto initialize_func =
        reinterpret_cast<decltype(&::Windows::Foundation::Initialize)>(
            GetProcAddress(handle, "Initialize"));

    if (initialize_func)
      hr_ = initialize_func(flags);
  }

  ~RoInitializeWrapper2() {
    if (SUCCEEDED(hr_)) {
      static HMODULE const handle = ::LoadLibrary(L"combase.dll");
      if (!handle)
        return;

      static const auto uninitialize_func =
          reinterpret_cast<decltype(&::Windows::Foundation::Uninitialize)>(
              GetProcAddress(handle, "Uninitialize"));

      if (uninitialize_func)
        uninitialize_func();
    }
  }

  operator HRESULT() { return hr_; }
};

namespace {

//  Name:     System.AppUserModel.ToastActivatorCLSID --
//            PKEY_AppUserModel_ToastActivatorCLSID
//  Type:     Guid -- VT_CLSID
//  FormatID: {9F4C2855-9F79-4B39-A8D0-E1D42DE1D5F3}, 26
//
//  Used to CoCreate an INotificationActivationCallback interface to notify
//  about toast activations.
EXTERN_C const PROPERTYKEY DECLSPEC_SELECTANY
    PKEY_AppUserModel_ToastActivatorCLSID = {
        {0x9F4C2855,
         0x9F79,
         0x4B39,
         {0xA8, 0xD0, 0xE1, 0xD4, 0x2D, 0xE1, 0xD5, 0xF3}},
        26};

// For the app to be activated from Action Center, it needs to provide a COM
// server to be called when the notification is activated. The CLSID of the COM
// object needs to be registered with the OS via the app shortcut.
//
// TODO(chengx): Different id for different browser distribution.
#define CLSID_KEY "E65AECC7-DD9B-4D14-A4ED-73A5BEF1187E"

class DECLSPEC_UUID(CLSID_KEY) NotificationActivator
    : public mswr::RuntimeClass<mswr::RuntimeClassFlags<mswr::ClassicCom>,
                                INotificationActivationCallback> {
 public:
  HRESULT STDMETHODCALLTYPE Activate(
      _In_ LPCWSTR /*appUserModelId*/,
      _In_ LPCWSTR invokedArgs,
      /*_In_reads_(dataCount)*/ const NOTIFICATION_USER_INPUT_DATA* /*data*/,
      ULONG /*dataCount*/) override {
    // TODO(chengx): Implement.
    ::MessageBoxW(NULL, L"activated from toast", L"", MB_OK);
    return S_OK;
  }
};
CoCreatableClass2(NotificationActivator);

// Register a COM server with the OS and register the CLSID of the COM server on
// the shortcut.
_Use_decl_annotations_ HRESULT
RegisterComServer(const base::FilePath& exe_path) {
  std::string key_path("Software\\Classes\\CLSID\\{");
  key_path.append(CLSID_KEY).append("}\\LocalServer32");

  base::win::RegKey key;
  if (key.Create(HKEY_LOCAL_MACHINE, base::UTF8ToUTF16(key_path).c_str(),
                 KEY_SET_VALUE) != ERROR_SUCCESS) {
    LOG(ERROR) << "Failed creating the reg key.";
    return S_FALSE;
  }

  base::string16 exe_path_str = exe_path.value();
  DWORD data_size =
      static_cast<DWORD>((::wcslen(exe_path_str.c_str()) + 1) * sizeof(WCHAR));

  // The first parameter must be nullptr for the app to be activated when it's
  // not running.
  if (key.WriteValue(nullptr,
                     reinterpret_cast<const BYTE*>(exe_path_str.c_str()),
                     data_size, REG_SZ) != ERROR_SUCCESS) {
    LOG(ERROR) << "Failed setting the value to the reg key.";
    return S_FALSE;
  }

  // TODO(chengx): Remove. For test purpose only.
  ::MessageBoxW(NULL, L"regkey succeed", L"Title", MB_OK);

  return S_OK;
}

// For the app to be able to be activated when it's not running, it needs to
// create a shortcut and have it installed on the Start menu. An AppUserModelID
// must be set on that shortcut. In addition, it needs to register a COM server
// with the OS and register the CLSID of that COM server on the shortcut.
// https://msdn.microsoft.com/en-us/library/windows/desktop/mt643715(v=vs.85).aspx
HRESULT RegisterAppForNativeNotification() {
  BrowserDistribution* dist = BrowserDistribution::GetDistribution();
  base::FilePath shortcut_path;

  if (!ShellUtil::GetShortcutPath(ShellUtil::SHORTCUT_LOCATION_START_MENU_ROOT,
                                  dist, ShellUtil::CURRENT_USER,
                                  &shortcut_path)) {
    LOG(ERROR) << "Failed finding the Start Menu shortcut directory.";
    return S_FALSE;
  }

  // This code snippet below doesn't work at least in local-buit Chrome. It
  // returns "Unable to find resource id" as in L60 in GetLocalizedString() in
  // l10n_string_util.cc
  //
  // shortcut_path =
  //    shortcut_path.Append(dist->GetShortcutName() + installer::kLnkExt);
  //
  // TODO(chengx): Then how to get the names for different Chrome channels?
  shortcut_path =
      shortcut_path.Append(base::string16(L"Chromium") + installer::kLnkExt);

  if (base::PathExists(shortcut_path)) {
    base::win::ShortcutProperties properties;
    base::win::ResolveShortcutProperties(
        shortcut_path, base::win::ShortcutProperties::PROPERTIES_CLSID,
        &properties);

    // If the existing shorcut has no clsid information, add it.
    if (properties.clsid == GUID_NULL) {
      base::win::ShortcutProperties shortcut_properties;
      shortcut_properties.set_clsid(__uuidof(NotificationActivator));

      if (!base::win::CreateOrUpdateShortcutLink(
              shortcut_path, shortcut_properties,
              base::win::SHORTCUT_UPDATE_EXISTING)) {
        LOG(ERROR) << "Failed updating the CLSID information for the shortcut.";
        return S_FALSE;
      }
    }

    return S_OK;
  }

  // Retrieve the path to chrome.exe.
  base::FilePath exe_path;
  if (!PathService::Get(base::FILE_EXE, &exe_path)) {
    NOTREACHED();
    LOG(ERROR) << "Failed retrieving the path to chrome.exe.";
    return S_FALSE;
  }

  // If no shortcut is found, create one.
  base::win::ShortcutProperties shortcut_properties;
  shortcut_properties.set_target(exe_path);
  shortcut_properties.set_clsid(__uuidof(NotificationActivator));

  base::string16 app_id =
      ShellUtil::GetBrowserModelId(InstallUtil::IsPerUserInstall());
  shortcut_properties.set_app_id(app_id);

  if (!base::win::CreateOrUpdateShortcutLink(
          shortcut_path, shortcut_properties,
          base::win::SHORTCUT_CREATE_ALWAYS)) {
    LOG(ERROR) << "Failed creating a new shortcut.";
    return S_FALSE;
  }

  return RegisterComServer(exe_path);
}

// Register activator for notifications so that it can be found by the system.
HRESULT RegisterActivator() {
  mswr::Module<mswr::OutOfProc>::Create([] {});
  mswr::Module<mswr::OutOfProc>::GetModule().IncrementObjectCount();
  return mswr::Module<mswr::OutOfProc>::GetModule().RegisterObjects();
}

// Unregister the activator.
void UnregisterActivator() {
  mswr::Module<mswr::OutOfProc>::GetModule().UnregisterObjects();
  mswr::Module<mswr::OutOfProc>::GetModule().DecrementObjectCount();
}

class ActivatorRegister {
 public:
  ActivatorRegister() {}
  ~ActivatorRegister() {
    if (has_registered_)
      UnregisterActivator();
  }

  HRESULT Run() {
    HRESULT hr = RegisterActivator();
    if (SUCCEEDED(hr))
      has_registered_ = true;
    return hr;
  }

 private:
  bool has_registered_ = false;
};

std::unique_ptr<ActivatorRegister> activator_register;

}  // namespace

void PrepareForNativeNotification() {
  RoInitializeWrapper2 winrt_initializer(RO_INIT_MULTITHREADED);
  if (FAILED(winrt_initializer)) {
    LOG(ERROR) << "Failed initializing the Windows Runtime.";
    return;
  }

  if (FAILED(RegisterAppForNativeNotification())) {
    LOG(ERROR) << "Failed registering the app for Windows native notification.";
    return;
  }

  activator_register.reset(new ActivatorRegister());
  activator_register->Run();
}
