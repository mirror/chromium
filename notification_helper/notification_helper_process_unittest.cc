// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <wrl/client.h>

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/process/process.h"
#include "base/process/process_iterator.h"
#include "base/strings/string16.h"
#include "base/test/test_timeouts.h"
#include "base/win/scoped_winrt_initializer.h"
#include "base/win/windows_version.h"
#include "chrome/install_static/install_util.h"
#include "chrome/installer/setup/setup_util.h"
#include "chrome/installer/util/install_util.h"
#include "chrome/installer/util/registry_key_backup.h"
#include "chrome/installer/util/util_constants.h"
#include "chrome/installer/util/work_item.h"
#include "chrome/installer/util/work_item_list.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// Returns the server process if it is found.
base::Process FindHelperProcess() {
  unsigned int helper_pid;
  {
    base::NamedProcessIterator iter(installer::kNotificationHelperExe, nullptr);
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
  base::NamedProcessIterator iter(installer::kNotificationHelperExe, nullptr);
  while (const auto* entry = iter.NextProcessEntry()) {
    if (entry->pid() == helper_pid)
      return helper;  // PID was not reused since the PID's match.
  }
  return base::Process();  // The PID was reused.
}

}  // namespace

class NotificationHelperTest : public testing::Test {
 protected:
  NotificationHelperTest()
      : toast_activator_reg_path_(installer::GetToastActivatorRegistryPath()),
        root_(HKEY_CURRENT_USER) {}

  void SetUp() override {
    // Backup the existing registration.
    ASSERT_TRUE(backup_.Initialize(root_, toast_activator_reg_path().c_str(),
                                   WorkItem::kWow64Default));
  }

  void TearDown() override {
    // Restore the registration.
    ASSERT_TRUE(backup_.WriteTo(root_, toast_activator_reg_path().c_str(),
                                WorkItem::kWow64Default));
  }

  // Deletes the registry key at Software\Classes\CLSID\|toast_activator_clsid|.
  void DeleteHelperRegistry() {
    ASSERT_TRUE(InstallUtil::DeleteRegistryKey(
        root_, toast_activator_reg_path(), WorkItem::kWow64Default));
  }

  // Registers notification_helper.exe in the build output directory as the
  // server.
  void RegisterServer() {
    // The path to the server exe. Notification_helper.exe should be in the
    // build output directory next to this test executable.
    base::FilePath dir_exe;
    ASSERT_TRUE(PathService::Get(base::DIR_EXE, &dir_exe));
    base::FilePath notification_helper =
        dir_exe.Append(installer::kNotificationHelperExe);

    base::string16 toast_activator_server_path = toast_activator_reg_path();
    toast_activator_server_path.append(L"\\LocalServer32");

    // Command-line featuring the quoted path to the exe.
    base::string16 command(1, L'"');
    command.append(notification_helper.value()).append(1, L'"');

    std::unique_ptr<WorkItemList> install_list(WorkItem::CreateWorkItemList());

    install_list->AddCreateRegKeyWorkItem(root_, toast_activator_server_path,
                                          WorkItem::kWow64Default);

    install_list->AddSetRegValueWorkItem(root_, toast_activator_server_path,
                                         WorkItem::kWow64Default, L"", command,
                                         true);

    install_list->AddSetRegValueWorkItem(
        root_, toast_activator_server_path, WorkItem::kWow64Default,
        L"ServerExecutable", notification_helper.value(), true);

    ASSERT_TRUE(install_list->Do());
  }

  const base::string16& toast_activator_reg_path() const {
    return toast_activator_reg_path_;
  }

 private:
  // Path to toast activator registry.
  const base::string16 toast_activator_reg_path_;

  // Predefined handle to registry key.
  const HKEY root_;

  // Backup of the deleted registry key.
  RegistryKeyBackup backup_;

  DISALLOW_COPY_AND_ASSIGN(NotificationHelperTest);
};

TEST_F(NotificationHelperTest, NotificationHelperServerTest) {
  // WinRT works on Windows 8 or above.
  if (base::win::GetVersion() < base::win::VERSION_WIN8)
    return;

  // Initialize WinRT.
  base::win::ScopedWinrtInitializer winrt_initializer;
  ASSERT_TRUE(winrt_initializer.Succeeded());

  DeleteHelperRegistry();

  // Register notification_helper.exe as the server. This requires to modify
  // the registry key. This is okay becasue we have already backed up the
  // existing registration in SetUp(), and will restore it in TearDown().
  RegisterServer();

  // The server has not been invoked yet at this point.
  base::Process helper = FindHelperProcess();
  ASSERT_FALSE(helper.IsValid());

  // Create a class instance.
  Microsoft::WRL::ComPtr<IUnknown> notification_activator;
  ASSERT_HRESULT_SUCCEEDED(::CoCreateInstance(
      install_static::GetToastActivatorClsid(), nullptr, CLSCTX_LOCAL_SERVER,
      IID_PPV_ARGS(&notification_activator)));
  ASSERT_TRUE(notification_activator);

  // The server is now invoked upon the request of creating the object instance.
  // The server module now holds a reference of the instance object, the
  // notification_helper.exe process is alive waiting for that reference to be
  // released.
  helper = FindHelperProcess();
  ASSERT_TRUE(helper.IsValid());

  // Release the instance object. Now that the last (and the only) instance
  // object of the module is released, the event living in the server
  // process is signaled, which allows the server process to exit.
  notification_activator.Reset();

  // Now the server process should already be freed.
  ASSERT_TRUE(
      helper.WaitForExitWithTimeout(TestTimeouts::action_timeout(), nullptr));
}
