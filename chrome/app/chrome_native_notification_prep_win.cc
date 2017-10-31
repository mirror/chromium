// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/app/chrome_native_notification_prep_win.h"

#include <NotificationActivationCallback.h>
#include <wrl.h>

#include "base/bind.h"
#include "base/threading/thread.h"
#include "base/win/registry.h"
#include "base/win/scoped_winrt_initializer.h"

// TODO: remove header files after moving RegisterComServer into installer.
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"

// TODO: Remove the 4 header files below.
#include "base/win/shortcut.h"
#include "chrome/installer/util/install_util.h"
#include "chrome/installer/util/shell_util.h"
#include "chrome/installer/util/util_constants.h"

namespace mswr = Microsoft::WRL;
namespace mswrd = Microsoft::WRL::Details;

const char kEnableNativeNotification[] = "enable-native-notification";

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

// A Win32 component that participates with Action Center will need to create a
// COM component that exposes the INotificationActivationCallback interface.
//
// TODO(chengx): Different id for different browser distribution.
#define CLSID_KEY "635EFA6F-08D6-4EC9-BD14-8A0FDE975159"

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
    return S_OK;
  }
};
CoCreatableClass2(NotificationActivator);

// TODO(chengx): Remove.
bool InstallShortcut() {
  BrowserDistribution* dist = BrowserDistribution::GetDistribution();
  base::FilePath shortcut_path;
  if (!ShellUtil::GetShortcutPath(ShellUtil::SHORTCUT_LOCATION_START_MENU_ROOT,
                                  dist, ShellUtil::CURRENT_USER,
                                  &shortcut_path)) {
    return false;
  }

  shortcut_path =
      shortcut_path.Append(base::string16(L"Chromium") + installer::kLnkExt);

  base::FilePath exe_path;
  if (!PathService::Get(base::FILE_EXE, &exe_path)) {
    NOTREACHED();
    return false;
  }

  base::win::ShortcutProperties shortcut_properties;
  shortcut_properties.set_target(exe_path);
  shortcut_properties.set_toast_activator_clsid(
      __uuidof(NotificationActivator));

  base::string16 app_id =
      ShellUtil::GetBrowserModelId(InstallUtil::IsPerUserInstall());
  shortcut_properties.set_app_id(app_id);

  return base::win::CreateOrUpdateShortcutLink(
      shortcut_path, shortcut_properties, base::win::SHORTCUT_CREATE_ALWAYS);
}

// Register a COM server with the OS, and tell it to start the app at |exe_path|
// when needed.
bool RegisterComServer() {
  std::string key_path("Software\\Classes\\CLSID\\{");
  key_path.append(CLSID_KEY).append("}\\LocalServer32");

  base::win::RegKey key;
  if (key.Create(HKEY_LOCAL_MACHINE, base::UTF8ToUTF16(key_path).c_str(),
                 KEY_SET_VALUE) != ERROR_SUCCESS) {
    return false;
  }

  // Retrieve the path to chrome.exe.
  base::FilePath exe_path;
  if (!PathService::Get(base::FILE_EXE, &exe_path)) {
    NOTREACHED();
    return false;
  }

  const wchar_t* exe_path_ptr = exe_path.value().c_str();
  DWORD data_size =
      static_cast<DWORD>((::wcslen(exe_path_ptr) + 1) * sizeof(WCHAR));

  // The first parameter must be nullptr for the app to be activated when it's
  // not running.
  return key.WriteValue(nullptr, reinterpret_cast<const BYTE*>(exe_path_ptr),
                        data_size, REG_SZ) == ERROR_SUCCESS;
}

// Register the COM object.
HRESULT RegisterActivator() {
  mswr::Module<mswr::OutOfProc>::Create([] {});
  mswr::Module<mswr::OutOfProc>::GetModule().IncrementObjectCount();
  return mswr::Module<mswr::OutOfProc>::GetModule().RegisterObjects();
}

// Unregister the COM object.
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

// For the app to be able to be activated when it's not running, it needs to
// have a shortcut installed on the Start menu. It also needs to register a COM
// server with the OS and register the CLSID of that COM server on the shortcut.
// https://msdn.microsoft.com/en-us/library/windows/desktop/mt643715(v=vs.85).aspx
void PrepareForNativeNotification() {
  base::win::ScopedWinrtInitializer scoped_winrt_initializer;
  if (!scoped_winrt_initializer.Succeeded())
    return;

  // TODO: Remove
  if (!InstallShortcut())
    return;

  if (!RegisterComServer()) {
    return;
  }

  activator_register.reset(new ActivatorRegister());
  activator_register->Run();
}

void StartNativeNotificationThread() {
  base::Thread notification_thread("Native Notification Thread");
  notification_thread.init_com_with_mta(true);
  notification_thread.Start();
  notification_thread.task_runner()->PostTask(
      FROM_HERE, base::Bind(&PrepareForNativeNotification));
}
