// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <memory>

#include <windows.h>

#include <atlbase.h>
#include <atlcom.h>
#include <atlctl.h>
#include <initguid.h>
#include <shellapi.h>
#include <wrl/client.h>

#include "base/at_exit.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/strings/string16.h"
#include "base/win/scoped_com_initializer.h"
#include "base/win/scoped_handle.h"
#include "base/win/windows_version.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/install_static/install_util.h"
#include "chrome/install_static/product_install_details.h"
#include "third_party/breakpad/breakpad/src/client/windows/handler/exception_handler.h"
#include "win10/delegate_execute/chrome_util.h"
#include "win10/delegate_execute/crash_server_init.h"
#include "win10/delegate_execute/resource.h"

namespace {

bool FindChromeExe(base::FilePath* chrome_exe) {
  // Look for chrome.exe one folder above delegate_execute.exe (as expected in
  // Chrome installs). Failing that, look for it alonside delegate_execute.exe.
  base::FilePath dir_exe;
  if (!PathService::Get(base::DIR_EXE, &dir_exe)) {
    AtlTrace("Failed to get current exe path\n");
    return false;
  }

  *chrome_exe = dir_exe.DirName().Append(chrome::kBrowserProcessExecutableName);
  if (!base::PathExists(*chrome_exe)) {
    *chrome_exe = dir_exe.Append(chrome::kBrowserProcessExecutableName);
    if (!base::PathExists(*chrome_exe)) {
      AtlTrace("Failed to find chrome exe file\n");
      return false;
    }
  }
  return true;
}

// Returns the name of the global event used to detect if |chrome_exe| is in
// use by a browser process.
// TODO(grt): Move this somewhere central so it can be used by both this
// IsBrowserRunning (below) and IsBrowserAlreadyRunning (browser_util_win.cc).
base::string16 GetEventName(const base::FilePath& chrome_exe) {
  static wchar_t const kEventPrefix[] = L"Global\\";
  const size_t prefix_len = arraysize(kEventPrefix) - 1;
  base::string16 name;
  name.reserve(prefix_len + chrome_exe.value().size());
  name.assign(kEventPrefix, prefix_len);
  name.append(chrome_exe.value());
  std::replace(name.begin() + prefix_len, name.end(), '\\', '!');
  std::transform(name.begin() + prefix_len, name.end(),
                 name.begin() + prefix_len, tolower);
  return name;
}

// Returns true if |chrome_exe| is in use by a browser process. In this case,
// "in use" means past ChromeBrowserMainParts::PreMainMessageLoopRunImpl.
bool IsBrowserRunning(const base::FilePath& chrome_exe) {
  base::win::ScopedHandle handle(
      ::OpenEvent(SYNCHRONIZE, FALSE, GetEventName(chrome_exe).c_str()));
  if (handle.IsValid())
    return true;
  DWORD last_error = ::GetLastError();
  if (last_error != ERROR_FILE_NOT_FOUND) {
    AtlTrace("%hs. Failed to open browser event; error %u.\n", __FUNCTION__,
             last_error);
  }
  return false;
}
int LaunchChrome() {
  if (base::win::GetVersion() >= base::win::VERSION_WIN10_RS1) {
    base::FilePath chrome_exe_path;
    bool found_exe = FindChromeExe(&chrome_exe_path);
    DCHECK(found_exe);

    if (found_exe) {
      if (IsBrowserRunning(chrome_exe_path))
        return S_OK;

      delegate_execute::UpdateChromeIfNeeded(chrome_exe_path);

      SHELLEXECUTEINFO sei = {sizeof(sei)};
      sei.fMask = SEE_MASK_FLAG_LOG_USAGE;
      sei.nShow = SW_SHOWNORMAL;
      sei.lpFile = chrome_exe_path.value().c_str();
      sei.lpParameters = NULL;

      AtlTrace(L"Launching Chrome via shortcut [%ls]\n", sei.lpFile);

      if (!::ShellExecuteExW(&sei)) {
        int error = HRESULT_FROM_WIN32(::GetLastError());
        AtlTrace("ShellExecute returned 0x%08X\n", error);
        return error;
      }
    }
  }
  return S_OK;
}

}  // namespace

using namespace ATL;

// Usually classes derived from CAtlExeModuleT, or other types of ATL
// COM module classes statically define their CLSID at compile time through
// the use of various macros, and ATL internals takes care of creating the
// class objects and registering them.  However, we need to register the same
// object with different CLSIDs depending on a runtime setting, so we handle
// that logic here, before the main ATL message loop runs.
class DelegateExecuteModule
    : public ATL::CAtlExeModuleT<DelegateExecuteModule> {
 public:
  typedef ATL::CAtlExeModuleT<DelegateExecuteModule> ParentClass;
  /*typedef CComObject<CommandExecuteImpl> ImplType;*/

  DelegateExecuteModule() : registration_token_(0) {}

  HRESULT PreMessageLoop(int nShowCmd) {
    HRESULT hr = S_OK;
    GUID clsid;
    const base::string16 clsid_string =
        install_static::GetLegacyCommandExecuteImplClsid();
    if (clsid_string.empty())
      return E_FAIL;
    hr = ::CLSIDFromString(clsid_string.c_str(), &clsid);
    if (FAILED(hr))
      return hr;

    //// We use the same class creation logic as ATL itself.  See
    //// _ATL_OBJMAP_ENTRY::RegisterClassObject() in atlbase.h
    // hr = ImplType::_ClassFactoryCreatorClass::CreateInstance(
    //    ImplType::_CreatorClass::CreateInstance, IID_IUnknown, &instance_);
    // if (FAILED(hr))
    //  return hr;
    // hr = ::CoRegisterClassObject(clsid, instance_.Get(), CLSCTX_LOCAL_SERVER,
    //                             REGCLS_MULTIPLEUSE | REGCLS_SUSPENDED,
    //                             &registration_token_);
    // if (FAILED(hr))
    //  return hr;

    return ParentClass::PreMessageLoop(nShowCmd);
  }

  int WinMain(int nShowCmd) {
    LaunchChrome();
    return ParentClass::WinMain(nShowCmd);
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

DelegateExecuteModule _AtlModule;

extern "C" int WINAPI wWinMain(HINSTANCE /* instance */,
                               HINSTANCE /* previous_instance */,
                               LPWSTR /* command_line */,
                               int nShowCmd) {
  install_static::InitializeProductDetailsForPrimaryModule();

  std::unique_ptr<google_breakpad::ExceptionHandler> breakpad =
      delegate_execute::InitializeCrashReporting();

  base::AtExitManager exit_manager;
  AtlTrace("delegate_execute enter\n");

  /*HRESULT ret_code = LaunchChrome();*/
  HRESULT ret_code = _AtlModule.WinMain(nShowCmd);

  AtlTrace("delegate_execute exit, code = %d\n", ret_code);
  return ret_code;
}