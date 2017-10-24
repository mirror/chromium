// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/app/chrome_native_notification_prep_win.h"

#include <NotificationActivationCallback.h>
#include <Pathcch.h>
#include <Shlobj.h>
#include <propkey.h>
#include <wrl.h>

#include "base/base_paths.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/strings/string16.h"
#include "base/win/shortcut.h"
#include "chrome/installer/util/install_util.h"
#include "chrome/installer/util/shell_util.h"
#include "chrome/installer/util/util_constants.h"

namespace mswr = Microsoft::WRL;
namespace mswrd = Microsoft::WRL::Details;
namespace mswrw = Microsoft::WRL::Wrappers;

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
// object needs to be registered with the OS via its shortcut.
//
// TODO(chengx): use a different GUID each time.
// #define __CLSID "E65AECC7-DD9B-4D14-A4ED-73A5BEF1187A"
class DECLSPEC_UUID("E65AECC7-DD9B-4D14-A4ED-73A5BEF1187A")
    NotificationActivator
    : public mswr::RuntimeClass<mswr::RuntimeClassFlags<mswr::ClassicCom>,
                                INotificationActivationCallback> {
 public:
  HRESULT STDMETHODCALLTYPE Activate(
      _In_ LPCWSTR /*appUserModelId*/,
      _In_ LPCWSTR invokedArgs,
      /*_In_reads_(dataCount)*/ const NOTIFICATION_USER_INPUT_DATA* /*data*/,
      ULONG /*dataCount*/) override {
    // TODO(chengx): implement
    ::MessageBoxW(NULL, L"activated from toast", L"", MB_OK);
    return S_OK;
  }
};
// Linker error for mswr::SimpleClassFactory if not including runtimeobject.lib
CoCreatableClass2(NotificationActivator);

_Use_decl_annotations_ HRESULT RegisterComServer(PCWSTR exe_path) {
  DWORD data_size =
      static_cast<DWORD>((::wcslen(exe_path) + 1) * sizeof(WCHAR));

  return HRESULT_FROM_WIN32(::RegSetKeyValue(
      HKEY_LOCAL_MACHINE,
      LR"(SOFTWARE\Classes\CLSID\{E65AECC7-DD9B-4D14-A4ED-73A5BEF1187A}"
            LR"\LocalServer32)",  // This needs to be the same as __CLSID
      nullptr, REG_SZ, reinterpret_cast<const BYTE*>(exe_path), data_size));
}

// For the app to be able to be activated at any time, it needs to create a
// shortcut and have it installed on the Start menu. This should happen as part
// of the install process for the app. An AppUserModelID must be set on that
// shortcut. In addition, it needs to register a COM server with the OS and
// register the CLSID of that COM server on the shortcut.
// https://msdn.microsoft.com/en-us/library/windows/desktop/mt643715(v=vs.85).aspx
HRESULT RegisterAppForNativeNotification() {
  base::FilePath shortcut_path;
  BrowserDistribution* dist = BrowserDistribution::GetDistribution();

  // TODO(chengx): CURRENT_USER or ?
  if (!ShellUtil::GetShortcutPath(ShellUtil::SHORTCUT_LOCATION_START_MENU_ROOT,
                                  dist, ShellUtil::CURRENT_USER,
                                  &shortcut_path)) {
    LOG(ERROR) << "Failed finding the Start Menu shortcut directory.";
    return S_FALSE;
  }

  // This code snippet cannot be used in local-buit Chrome. It returns "Unable
  // to find resource id" as in L60 in GetLocalizedString() in
  // l10n_string_util.cc
  //
  // However, it should be turned on for production. Any alternative for local
  // testing purpose?
  //
  // shortcut_path =
  //    shortcut_path.Append(dist->GetShortcutName() + installer::kLnkExt);
  shortcut_path = shortcut_path.Append(base::string16(L"Chromium_test") +
                                       installer::kLnkExt);
  // ::MessageBoxW(NULL, shortcut_path.value().c_str(), "Title", MB_OK);

  if (base::PathExists(shortcut_path)) {
    base::win::ShortcutProperties properties;
    base::win::ResolveShortcutProperties(
        shortcut_path, base::win::ShortcutProperties::PROPERTIES_CLSID,
        &properties);

    // If the existing shorcut has no clsid information, update it
    if (properties.clsid == GUID_NULL) {
      base::win::ShortcutProperties shortcut_properties;
      shortcut_properties.set_clsid(__uuidof(NotificationActivator));

      base::win::CreateOrUpdateShortcutLink(
          shortcut_path, shortcut_properties,
          base::win::SHORTCUT_UPDATE_EXISTING);
    }

    return S_OK;
  }

  // Retrieve the path to chrome.exe
  base::FilePath exe_path;
  if (!PathService::Get(base::FILE_EXE, &exe_path)) {
    NOTREACHED();
    return S_FALSE;
  }

  // If no shortcut is found, create one
  base::win::ShortcutProperties shortcut_properties;
  shortcut_properties.set_target(exe_path);
  shortcut_properties.set_clsid(__uuidof(NotificationActivator));

  base::string16 app_id =
      ShellUtil::GetBrowserModelId(InstallUtil::IsPerUserInstall());
  shortcut_properties.set_app_id(app_id);

  if (!base::win::CreateOrUpdateShortcutLink(
          shortcut_path, shortcut_properties,
          base::win::SHORTCUT_CREATE_ALWAYS)) {
    return S_FALSE;
  }

  // TODO(chengx): remove, for test purpose
  /*MessageBoxW(NULL, L"Create new shortcut succeeds", L"Title", MB_OK);*/

  return RegisterComServer(exe_path.value().c_str());
}  // namespace

// Register activator for notifications so that it can be found by the system.
HRESULT RegisterActivator() {
  // This causes linker errors if w/o including runtimeobject.lib
  mswr::Module<mswr::OutOfProc>::Create([] {});
  mswr::Module<mswr::OutOfProc>::GetModule().IncrementObjectCount();
  return mswr::Module<mswr::OutOfProc>::GetModule().RegisterObjects();
}

// Unregister the activator
void UnregisterActivator() {
  // This causes linker errors if w/o including runtimeobject.lib
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

// RoInitializeWrapper2 reimplements mswrw::RoInitializeWrapper in
// wrl\wrappers\corewrappers.h with the LoadLibrary/GetProcAddress strategy.
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

void PrepareForNativeNotification() {
  RoInitializeWrapper2 winrt_initializer(RO_INIT_MULTITHREADED);
  if (FAILED(winrt_initializer))
    return;

  if (FAILED(RegisterAppForNativeNotification())
    return;

  activator_register.reset(new ActivatorRegister());
  activator_register->Run();
}
