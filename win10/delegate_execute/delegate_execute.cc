// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <atlbase.h>
#include <atlcom.h>
#include <atlctl.h>
#include <initguid.h>
#include <shellapi.h>
#include <wrl/client.h>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/process/kill.h"
#include "base/strings/string16.h"
#include "base/win/scoped_com_initializer.h"
#include "base/win/scoped_handle.h"
#include "base/win/windows_version.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/install_static/install_util.h"
#include "third_party/breakpad/breakpad/src/client/windows/handler/exception_handler.h"
#include "win10/delegate_execute/crash_server_init.h"
#include "win10/delegate_execute/resource.h"
using namespace ATL;
using base::win::ScopedHandle;

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
int LaunchChrome() {
  base::win::Version version = base::win::GetVersion();
  if (version >= base::win::VERSION_WIN10_RS1) {
    base::win::ScopedCOMInitializer com_initializer;

    base::FilePath chrome_exe_path;
    bool found_exe = FindChromeExe(&chrome_exe_path);
    DCHECK(found_exe);

    if (found_exe) {
      SHELLEXECUTEINFO sei = {sizeof(sei)};
      sei.fMask = SEE_MASK_FLAG_LOG_USAGE;
      sei.nShow = SW_SHOWNORMAL;
      // For test, L"C:\\Program Files
      // (x86)\\Google\\Chrome\\Application\\chrome.exe";
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

extern "C" int WINAPI _tWinMain(HINSTANCE, HINSTANCE, LPTSTR, int nShowCmd) {
  // std::unique_ptr<google_breakpad::ExceptionHandler> breakpad =
  //    delegate_execute::InitializeCrashReporting();

  base::AtExitManager exit_manager;
  AtlTrace("delegate_execute enter\n");

  HRESULT ret_code = LaunchChrome();

  AtlTrace("delegate_execute exit, code = %d\n", ret_code);
  return ret_code;
}