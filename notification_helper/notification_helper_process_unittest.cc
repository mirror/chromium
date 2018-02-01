// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <windows.h>

#include <cstdio>

#include <NotificationActivationCallback.h>
#include <objbase.h>
#include <tlhelp32.h>

#include "base/win/scoped_winrt_initializer.h"
#include "chrome/install_static/install_util.h"
#include "chrome/install_static/product_install_details.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// This must be consistent with kNotificationHelperExe in
// "chrome/installer/setup/setup_constants.h".
constexpr wchar_t kNotificationHelperExe[] = L"notification_helper.exe";

// The time that the thread will sleep before the process running status check.
constexpr int kSleepTimeMilliseconds = 3000;

// Checks if a process named |process_name| is running.
bool IsProcessRunning(const wchar_t* process_name) {
  bool is_running = false;
  PROCESSENTRY32 entry;
  entry.dwSize = sizeof(entry);

  HANDLE snapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
  if (snapshot == INVALID_HANDLE_VALUE)
    return is_running;

  if (::Process32First(snapshot, &entry)) {
    while (::Process32Next(snapshot, &entry)) {
      if (!wcsicmp(entry.szExeFile, process_name))
        is_running = true;
    }
  }

  ::CloseHandle(snapshot);
  return is_running;
}

}  // namespace

TEST(NotificationHelper, NotificationHelperServerTest) {
  install_static::InitializeProductDetailsForPrimaryModule();

  // Initialize WinRT.
  base::win::ScopedWinrtInitializer winrt_initializer;
  ASSERT_TRUE(winrt_initializer.Succeeded());

  // Create a class instance.
  // We could use Microsoft::WRL::ComPtr to hold notification_activator.
  // However, this requires including <wrl/client.h>, which surprisingly
  // violates checkdeps rules.
  INotificationActivationCallback* notification_activator;
  HRESULT hr = ::CoCreateInstance(install_static::GetToastActivatorClsid(),
                                  nullptr, CLSCTX_LOCAL_SERVER,
                                  IID_PPV_ARGS(&notification_activator));

  // It is possible that the machine has not installed Chrome/Chromium. In this
  // case, CoCreateInstance will complain that the class is not registered in
  // the registration database.
  if (hr == REGDB_E_CLASSNOTREG) {
    ASSERT_FALSE(notification_activator);
    return;
  }

  EXPECT_TRUE(SUCCEEDED(hr));
  ASSERT_TRUE(notification_activator);

  // notification_helper.exe will be invoked upon the request of creating the
  // object instance. The module now holds a reference of the instance object,
  // the notification_helper.exe process is alive waiting for that reference
  // to be released.
  ASSERT_TRUE(IsProcessRunning(kNotificationHelperExe));

  // Release the instance object. Now that the last (and the only) instance
  // object of the module is released, the event living in
  // notification_helper.exe is signaled. The notification_helper.exe
  // process will be freed.
  notification_activator->Release();

  // Suspend the execution of the current thread to make sure that
  // notification_helper.exe has exited.
  ::Sleep(kSleepTimeMilliseconds);

  // Now notification_helper.exe process should already be freed.
  ASSERT_FALSE(IsProcessRunning(kNotificationHelperExe));
}
