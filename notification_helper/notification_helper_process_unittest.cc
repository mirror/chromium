// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <windows.h>

#include <cstdio>

#include <NotificationActivationCallback.h>
#include <objbase.h>
#include <tlhelp32.h>
#include <wrl/client.h>

#include "base/process/process.h"
#include "base/process/process_iterator.h"
#include "base/test/test_timeouts.h"
#include "base/win/scoped_handle.h"
#include "base/win/scoped_winrt_initializer.h"
#include "chrome/install_static/install_util.h"
#include "chrome/install_static/product_install_details.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// This must be consistent with kNotificationHelperExe in
// "chrome/installer/setup/setup_constants.h".
constexpr wchar_t kNotificationHelperExe[] = L"notification_helper.exe";

// Returns the notification_helper process if it is found.
base::Process FindHelperProcess() {
  unsigned int helper_pid = 0;
  {
    base::NamedProcessIterator iter(kNotificationHelperExe, nullptr);
    const auto* entry = iter.NextProcessEntry();
    if (!entry)
      return base::Process();
    helper_pid = entry->pid();
  }
  base::Process helper = base::Process::Open(helper_pid);
  if (!helper.IsValid())
    return helper;

  // Due to aggressive PID reuse, it's possible that a different process was
  // just opened. Now that a handle is held to *some* process, take another
  // run through the snapshot to see if the process with this PID has the right
  // exe name.
  base::NamedProcessIterator iter(kNotificationHelperExe, nullptr);
  while (const auto* entry = iter.NextProcessEntry()) {
    if (entry->pid() == helper_pid)
      return helper;  // Found it.
  }
  return base::Process();  // The PID was reused.
}

}  // namespace

TEST(NotificationHelper, NotificationHelperServerTest) {
  install_static::InitializeProductDetailsForPrimaryModule();

  // Initialize WinRT.
  base::win::ScopedWinrtInitializer winrt_initializer;
  ASSERT_TRUE(winrt_initializer.Succeeded());

  // Notification_helper.exe has not been invoked yet at this point.
  base::Process helper = FindHelperProcess();
  ASSERT_FALSE(helper.IsValid());

  // Create a class instance.
  Microsoft::WRL::ComPtr<IUnknown> notification_activator;
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

  // Notification_helper.exe is now invoked upon the request of creating the
  // object instance. The module now holds a reference of the instance object,
  // the notification_helper.exe process is alive waiting for that reference
  // to be released.
  helper = FindHelperProcess();
  ASSERT_TRUE(helper.IsValid());

  // Release the instance object. Now that the last (and the only) instance
  // object of the module is released, the event living in
  // notification_helper.exe is signaled. The notification_helper.exe
  // process will be freed.
  notification_activator->Release();

  // Now the notification_helper.exe process should already be freed.
  ASSERT_TRUE(
      helper.WaitForExitWithTimeout(TestTimeouts::action_timeout(), nullptr));
}