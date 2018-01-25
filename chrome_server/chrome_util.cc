// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome_server/chrome_util.h"

#include <atltrace.h>
#include <shellapi.h>

#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/win/scoped_handle.h"
#include "chrome/common/chrome_constants.h"

using namespace ATL;

namespace {

// Returns the file path of Chrome.exe if found, or an empty file path if not.
base::FilePath GetChromeExePath() {
  // Look for chrome.exe one folder above chrome_server.exe (as expected in
  // Chrome installs). Failing that, look for it alonside chrome_server.exe.
  base::FilePath dir_exe;
  if (!PathService::Get(base::DIR_EXE, &dir_exe)) {
    AtlTrace("Failed to get current exe path\n");
    return base::FilePath();
  }

  base::FilePath chrome_exe =
      dir_exe.DirName().Append(chrome::kBrowserProcessExecutableName);

  if (!base::PathExists(chrome_exe)) {
    chrome_exe = dir_exe.Append(chrome::kBrowserProcessExecutableName);
    if (!base::PathExists(chrome_exe)) {
      AtlTrace("Failed to find chrome exe file\n");
      return base::FilePath();
    }
  }
  return chrome_exe;
}

// Returns the name of the global event used to detect if |chrome_exe| is in
// use by a browser process.
// The body of this method must be kept in sync with that of
// browser_util::IsBrowserAlreadyRunning.
base::string16 GetEventName(const base::FilePath& chrome_exe) {
  static constexpr wchar_t kEventPrefix[] = L"Global\\";
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

}  // namespace

namespace chrome_server {

int LaunchChromeIfNotRunning() {
  base::FilePath chrome_exe_path = GetChromeExePath();

  if (!chrome_exe_path.empty() && !IsBrowserRunning(chrome_exe_path)) {
    SHELLEXECUTEINFO sei = {sizeof(sei)};
    sei.fMask = SEE_MASK_NOASYNC | SEE_MASK_FLAG_LOG_USAGE;
    sei.nShow = SW_SHOWNORMAL;
    sei.lpFile = chrome_exe_path.value().c_str();

    AtlTrace(L"Launching Chrome via chrome_server.exe [%ls]\n", sei.lpFile);

    if (!::ShellExecuteExW(&sei)) {
      int error = HRESULT_FROM_WIN32(::GetLastError());
      AtlTrace("ShellExecute returned 0x%08X\n", error);
      return error;
    }
  }

  return S_OK;
}

}  // namespace chrome_server
