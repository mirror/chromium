// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "chrome/app/chrome_native_notification_prep_win.h"

#include <NotificationActivationCallback.h>
#include <Pathcch.h>
#include <Shlobj.h>
#include <propkey.h>
#include <wrl.h>

#include "chrome/installer/util/install_util.h"
#include "chrome/installer/util/shell_util.h"

namespace mswr = Microsoft::WRL;
namespace mswrd = Microsoft::WRL::Details;
namespace mswrw = Microsoft::WRL::Wrappers;

namespace {

// The three macros below are redefinitions of CoCreatableClass,
// CoCreatableClassWithFactory, and InternalWrlCreateCreatorMap in
// /winrt/wrl/module.h
//
// The original definitions caused win_clang complie errors. To fix the errors,
// we removed __declspec(selectany) in InternalWrlCreateCreatorMap, and added
// braces to "0" and "runtimeClassName" in InternalWrlCreateCreatorMap.

#define CoCreatableClass2(className) \
  CoCreatableClassWithFactory2(className, mswr::SimpleClassFactory<className>)

#define CoCreatableClassWithFactory2(className, factory)                       \
  InternalWrlCreateCreatorMap2(className##_COM, &__uuidof(className), nullptr, \
                               mswrd::CreateClassFactory<factory>,             \
                               "minATL$__f")

#define InternalWrlCreateCreatorMap2(className, runtimeClassName, trustLevel, \
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

struct CoTaskMemStringTraits {
  typedef PWSTR Type;

  inline static bool Close(_In_ Type h) throw() {
    ::CoTaskMemFree(h);
    return true;
  }

  inline static Type GetInvalidValue() throw() { return nullptr; }
};
typedef mswrw::HandleT<CoTaskMemStringTraits> CoTaskMemString;

// TODO(chengx): use a different GUID each time.
#define __CLSID "E65AECC7-DD9B-4D14-A4ED-73A5BEF1187A"

// For the app to be activated from Action Center, it needs to provide a COM
// server to be called when the notification is activated. The CLSID of the COM
// object needs to be registered with the OS via its shortcut.
class DECLSPEC_UUID(__CLSID) NotificationActivator
    : public mswr::RuntimeClass<mswr::RuntimeClassFlags<mswr::ClassicCom>,
                                INotificationActivationCallback> {
 public:
  HRESULT STDMETHODCALLTYPE Activate(
      _In_ LPCWSTR /*appUserModelId*/,
      _In_ LPCWSTR invokedArgs,
      /*_In_reads_(dataCount)*/ const NOTIFICATION_USER_INPUT_DATA* /*data*/,
      ULONG /*dataCount*/) override {
    // TODO(chengx): implement
    return S_OK;
  }
};
// CoCreatableClass2(NotificationActivator);

// This method creates a shortcut and assigns the AppUserModelID and
// ToastActivatorCLSID properties.
_Use_decl_annotations_ HRESULT InstallShortcut(PCWSTR shortcutPath,
                                               PCWSTR exePath) {
  mswr::ComPtr<IShellLink> shellLink;
  HRESULT hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&shellLink));
  if (FAILED(hr))
    return hr;

  hr = shellLink->SetPath(exePath);
  if (FAILED(hr))
    return hr;

  mswr::ComPtr<IPropertyStore> propertyStore;
  hr = shellLink.As(&propertyStore);
  if (FAILED(hr))
    return hr;

  PROPVARIANT propVar;
  propVar.vt = VT_LPWSTR;

  // Get AppUserModelId for Chrome
  propVar.pwszVal = const_cast<PWSTR>(
      ShellUtil::GetBrowserModelId(InstallUtil::IsPerUserInstall()).c_str());
  hr = propertyStore->SetValue(PKEY_AppUserModel_ID, propVar);
  if (FAILED(hr))
    return hr;

  propVar.vt = VT_CLSID;
  propVar.puuid = const_cast<CLSID*>(&__uuidof(NotificationActivator));
  hr = propertyStore->SetValue(PKEY_AppUserModel_ToastActivatorCLSID, propVar);
  if (FAILED(hr))
    return hr;

  hr = propertyStore->Commit();
  if (FAILED(hr))
    return hr;

  mswr::ComPtr<IPersistFile> persistFile;
  hr = shellLink.As(&persistFile);
  if (FAILED(hr))
    return hr;

  hr = persistFile->Save(shortcutPath, TRUE);
  return hr;
}

_Use_decl_annotations_ HRESULT RegisterComServer(PCWSTR exePath) {
  DWORD dataSize = static_cast<DWORD>((::wcslen(exePath) + 1) * sizeof(WCHAR));

  return HRESULT_FROM_WIN32(::RegSetKeyValue(
      HKEY_CURRENT_USER,
      LR"(SOFTWARE\Classes\CLSID\{E65AECC7-DD9B-4D14-A4ED-73A5BEF1187A}"
            LR"\LocalServer32)",  // This needs to be the same as __CLSID
      nullptr, REG_SZ, reinterpret_cast<const BYTE*>(exePath), dataSize));
}

// For the app to be able to be activated at any time, it needs to create a
// shortcut and have it installed on the Start menu. This should happen as part
// of the install process for the app. An AppUserModelID must be set on that
// shortcut. In addition, it needs to register a COM server with the OS and
// register the CLSID of that COM server on the shortcut.
// https://msdn.microsoft.com/en-us/library/windows/desktop/mt643715(v=vs.85).aspx
HRESULT RegisterAppForNativeNotification() {
  CoTaskMemString appData;
  // TODO(chengx): DIR_START_MENU (currently used) or DIR_COMMON_START_MENU?
  auto hr = ::SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr,
                                   appData.GetAddressOf());
  if (FAILED(hr))
    return hr;

  wchar_t shortcutPath[MAX_PATH];

  // TODO(chengx): Replace the test shortcut with Chrome shortcut
  // (Stable/Beta/etc) via ShellUtil::GetShortcutPath().
  hr = ::PathCchCombine(
      shortcutPath, ARRAYSIZE(shortcutPath), appData.Get(),
      LR"(Microsoft\Windows\Start Menu\Programs\test_notification.lnk)");
  if (FAILED(hr))
    return hr;

  DWORD attributes = ::GetFileAttributes(shortcutPath);
  bool fileExists = attributes < 0xFFFFFFF;

  if (fileExists) {
    // TODO(chengx): remove, for test purpose
    MessageBoxA(NULL, "Registered already", "Title", MB_OK);
    return hr;
  }

  wchar_t exePath[MAX_PATH];
  DWORD charWritten = ::GetModuleFileName(nullptr, exePath, ARRAYSIZE(exePath));
  hr = charWritten > 0 ? S_OK : HRESULT_FROM_WIN32(::GetLastError());
  if (FAILED(hr))
    return hr;

  hr = InstallShortcut(shortcutPath, exePath);
  if (FAILED(hr))
    return hr;

  // TODO(chengx): remove, for test purpose
  MessageBoxW(NULL, L"InstallShortcut() succeeds", L"Title", MB_OK);
  hr = RegisterComServer(exePath);
  return hr;
}

// Register activator for notifications so that it can be found by the system.
HRESULT RegisterActivator() {
  /*mswr::Module<mswr::OutOfProc>::Create([] {});
  mswr::Module<mswr::OutOfProc>::GetModule().IncrementObjectCount();*/
  /*return mswr::Module<mswr::OutOfProc>::GetModule().RegisterObjects();*/
  return S_OK;
}

// Unregister the activator
void UnregisterActivator() {
  /*mswr::Module<mswr::OutOfProc>::GetModule().UnregisterObjects();
  mswr::Module<mswr::OutOfProc>::GetModule().DecrementObjectCount();*/
}

class ActivatorRegister {
 public:
  ActivatorRegister() {}
  ~ActivatorRegister() {
    if (has_reg)
      UnregisterActivator();
  }

  HRESULT Run() {
    HRESULT hr = RegisterActivator();
    if (SUCCEEDED(hr))
      has_reg = true;
    return hr;
  }

 private:
  bool has_reg = false;
};

std::unique_ptr<ActivatorRegister> activator_reg;

}  // namespace

void PrepareForNativeNotification() {
  /*mswrw::RoInitializeWrapper winRtInitializer(RO_INIT_MULTITHREADED);
  if (FAILED(winRtInitializer))
    return;*/

  auto hr = RegisterAppForNativeNotification();
  if (FAILED(hr))
    return;

  activator_reg.reset(new ActivatorRegister());
  hr = activator_reg->Run();

  // TODO(chengx): remove, for test purpose
  if (SUCCEEDED(hr))
    ::MessageBoxA(NULL, "preparation succeeds", "Title", MB_OK);
}
