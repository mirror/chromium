// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// See the corresponding header file for description of the functions in this
// file.

#include "chrome/installer/util/install_util.h"

#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>

#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "base/process/launch.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/sys_info.h"
#include "base/values.h"
#include "base/version.h"
#include "base/win/registry.h"
#include "base/win/windows_version.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/install_static/install_details.h"
#include "chrome/install_static/install_modes.h"
#include "chrome/install_static/install_util.h"
#include "chrome/installer/util/browser_distribution.h"
#include "chrome/installer/util/google_update_constants.h"
#include "chrome/installer/util/installation_state.h"
#include "chrome/installer/util/l10n_string_util.h"
#include "chrome/installer/util/util_constants.h"
#include "chrome/installer/util/work_item_list.h"

using base::win::RegKey;
using installer::ProductState;

namespace {

const wchar_t kRegDowngradeVersion[] = L"DowngradeVersion";

// Creates a zero-sized non-decorated foreground window that doesn't appear
// in the taskbar. This is used as a parent window for calls to ShellExecuteEx
// in order for the UAC dialog to appear in the foreground and for focus
// to be returned to this process once the UAC task is dismissed. Returns
// NULL on failure, a handle to the UAC window on success.
HWND CreateUACForegroundWindow() {
  HWND foreground_window = ::CreateWindowEx(WS_EX_TOOLWINDOW,
                                            L"STATIC",
                                            NULL,
                                            WS_POPUP | WS_VISIBLE,
                                            0, 0, 0, 0,
                                            NULL, NULL,
                                            ::GetModuleHandle(NULL),
                                            NULL);
  if (foreground_window) {
    HMONITOR monitor = ::MonitorFromWindow(foreground_window,
                                           MONITOR_DEFAULTTONEAREST);
    if (monitor) {
      MONITORINFO mi = {0};
      mi.cbSize = sizeof(mi);
      ::GetMonitorInfo(monitor, &mi);
      RECT screen_rect = mi.rcWork;
      int x_offset = (screen_rect.right - screen_rect.left) / 2;
      int y_offset = (screen_rect.bottom - screen_rect.top) / 2;
      ::MoveWindow(foreground_window,
                   screen_rect.left + x_offset,
                   screen_rect.top + y_offset,
                   0, 0, FALSE);
    } else {
      NOTREACHED() << "Unable to get default monitor";
    }
    ::SetForegroundWindow(foreground_window);
  }
  return foreground_window;
}

}  // namespace

void InstallUtil::TriggerActiveSetupCommand() {
  base::string16 active_setup_reg(install_static::GetActiveSetupPath());
  base::win::RegKey active_setup_key(
      HKEY_LOCAL_MACHINE, active_setup_reg.c_str(), KEY_QUERY_VALUE);
  base::string16 cmd_str;
  LONG read_status = active_setup_key.ReadValue(L"StubPath", &cmd_str);
  if (read_status != ERROR_SUCCESS) {
    LOG(ERROR) << active_setup_reg << ", " << read_status;
    // This should never fail if Chrome is registered at system-level, but if it
    // does there is not much else to be done.
    return;
  }

  base::CommandLine cmd(base::CommandLine::FromString(cmd_str));
  // Force creation of shortcuts as the First Run beacon might land between now
  // and the time setup.exe checks for it.
  cmd.AppendSwitch(installer::switches::kForceConfigureUserSettings);

  base::Process process =
      base::LaunchProcess(cmd.GetCommandLineString(), base::LaunchOptions());
  if (!process.IsValid())
    PLOG(ERROR) << cmd.GetCommandLineString();
}

bool InstallUtil::ExecuteExeAsAdmin(const base::CommandLine& cmd,
                                    DWORD* exit_code) {
  base::FilePath::StringType program(cmd.GetProgram().value());
  DCHECK(!program.empty());
  DCHECK_NE(program[0], L'\"');

  base::CommandLine::StringType params(cmd.GetCommandLineString());
  if (params[0] == '"') {
    DCHECK_EQ('"', params[program.length() + 1]);
    DCHECK_EQ(program, params.substr(1, program.length()));
    params = params.substr(program.length() + 2);
  } else {
    DCHECK_EQ(program, params.substr(0, program.length()));
    params = params.substr(program.length());
  }

  base::TrimWhitespace(params, base::TRIM_ALL, &params);

  HWND uac_foreground_window = CreateUACForegroundWindow();

  SHELLEXECUTEINFO info = {0};
  info.cbSize = sizeof(SHELLEXECUTEINFO);
  info.fMask = SEE_MASK_NOCLOSEPROCESS;
  info.hwnd = uac_foreground_window;
  info.lpVerb = L"runas";
  info.lpFile = program.c_str();
  info.lpParameters = params.c_str();
  info.nShow = SW_SHOW;

  bool success = false;
  if (::ShellExecuteEx(&info) == TRUE) {
    ::WaitForSingleObject(info.hProcess, INFINITE);
    DWORD ret_val = 0;
    if (::GetExitCodeProcess(info.hProcess, &ret_val)) {
      success = true;
      if (exit_code)
        *exit_code = ret_val;
    }
  }

  if (uac_foreground_window) {
    DestroyWindow(uac_foreground_window);
  }

  return success;
}

base::CommandLine InstallUtil::GetChromeUninstallCmd(bool system_install) {
  ProductState state;
  if (state.Initialize(system_install))
    return state.uninstall_command();
  return base::CommandLine(base::CommandLine::NO_PROGRAM);
}

void InstallUtil::GetChromeVersion(BrowserDistribution* dist,
                                   bool system_install,
                                   base::Version* version) {
  DCHECK(dist);
  RegKey key;
  HKEY reg_root = (system_install) ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
  LONG result = key.Open(reg_root,
                         dist->GetVersionKey().c_str(),
                         KEY_QUERY_VALUE | KEY_WOW64_32KEY);

  base::string16 version_str;
  if (result == ERROR_SUCCESS)
    result = key.ReadValue(google_update::kRegVersionField, &version_str);

  *version = base::Version();
  if (result == ERROR_SUCCESS && !version_str.empty()) {
    VLOG(1) << "Existing " << dist->GetDisplayName() << " version found "
            << version_str;
    *version = base::Version(base::UTF16ToASCII(version_str));
  } else {
    DCHECK_EQ(ERROR_FILE_NOT_FOUND, result);
    VLOG(1) << "No existing " << dist->GetDisplayName()
            << " install found.";
  }
}

void InstallUtil::GetCriticalUpdateVersion(BrowserDistribution* dist,
                                           bool system_install,
                                           base::Version* version) {
  DCHECK(dist);
  RegKey key;
  HKEY reg_root = (system_install) ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
  LONG result = key.Open(reg_root,
                         dist->GetVersionKey().c_str(),
                         KEY_QUERY_VALUE | KEY_WOW64_32KEY);

  base::string16 version_str;
  if (result == ERROR_SUCCESS)
    result = key.ReadValue(google_update::kRegCriticalVersionField,
                           &version_str);

  *version = base::Version();
  if (result == ERROR_SUCCESS && !version_str.empty()) {
    VLOG(1) << "Critical Update version for " << dist->GetDisplayName()
            << " found " << version_str;
    *version = base::Version(base::UTF16ToASCII(version_str));
  } else {
    DCHECK_EQ(ERROR_FILE_NOT_FOUND, result);
    VLOG(1) << "No existing " << dist->GetDisplayName()
            << " install found.";
  }
}

bool InstallUtil::IsOSSupported() {
  // We do not support anything prior to Windows 7.
  VLOG(1) << base::SysInfo::OperatingSystemName() << ' '
          << base::SysInfo::OperatingSystemVersion();
  return base::win::GetVersion() >= base::win::VERSION_WIN7;
}

void InstallUtil::AddInstallerResultItems(
    bool system_install,
    const base::string16& state_key,
    installer::InstallStatus status,
    int string_resource_id,
    const base::string16* const launch_cmd,
    WorkItemList* install_list) {
  DCHECK(install_list);
  DCHECK(install_list->best_effort());
  DCHECK(!install_list->rollback_enabled());

  const HKEY root = system_install ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
  DWORD installer_result = (GetInstallReturnCode(status) == 0) ? 0 : 1;
  install_list->AddCreateRegKeyWorkItem(root, state_key, KEY_WOW64_32KEY);
  install_list->AddSetRegValueWorkItem(root,
                                       state_key,
                                       KEY_WOW64_32KEY,
                                       installer::kInstallerResult,
                                       installer_result,
                                       true);
  install_list->AddSetRegValueWorkItem(root,
                                       state_key,
                                       KEY_WOW64_32KEY,
                                       installer::kInstallerError,
                                       static_cast<DWORD>(status),
                                       true);
  if (string_resource_id != 0) {
    base::string16 msg = installer::GetLocalizedString(string_resource_id);
    install_list->AddSetRegValueWorkItem(root,
                                         state_key,
                                         KEY_WOW64_32KEY,
                                         installer::kInstallerResultUIString,
                                         msg,
                                         true);
  }
  if (launch_cmd != NULL && !launch_cmd->empty()) {
    install_list->AddSetRegValueWorkItem(
        root,
        state_key,
        KEY_WOW64_32KEY,
        installer::kInstallerSuccessLaunchCmdLine,
        *launch_cmd,
        true);
  }
}

bool InstallUtil::IsPerUserInstall() {
  return !install_static::InstallDetails::Get().system_level();
}

// static
bool InstallUtil::IsFirstRunSentinelPresent() {
  // TODO(msw): Consolidate with first_run::internal::IsFirstRunSentinelPresent.
  base::FilePath user_data_dir;
  return !PathService::Get(chrome::DIR_USER_DATA, &user_data_dir) ||
         base::PathExists(user_data_dir.Append(chrome::kFirstRunSentinel));
}

// static
bool InstallUtil::GetEULASentinelFilePath(base::FilePath* path) {
  base::FilePath user_data_dir;
  if (!PathService::Get(chrome::DIR_USER_DATA, &user_data_dir))
    return false;
  *path = user_data_dir.Append(installer::kEULASentinelFile);
  return true;
}

// This method tries to delete a registry key and logs an error message
// in case of failure. It returns true if deletion is successful (or the key did
// not exist), otherwise false.
bool InstallUtil::DeleteRegistryKey(HKEY root_key,
                                    const base::string16& key_path,
                                    REGSAM wow64_access) {
  VLOG(1) << "Deleting registry key " << key_path;
  RegKey target_key;
  LONG result =
      target_key.Open(root_key, key_path.c_str(), DELETE | wow64_access);

  if (result == ERROR_FILE_NOT_FOUND)
    return true;

  if (result == ERROR_SUCCESS)
    result = target_key.DeleteKey(L"");

  if (result != ERROR_SUCCESS) {
    LOG(ERROR) << "Failed to delete registry key: " << key_path
               << " error: " << result;
    return false;
  }
  return true;
}

// This method tries to delete a registry value and logs an error message
// in case of failure. It returns true if deletion is successful (or the key did
// not exist), otherwise false.
bool InstallUtil::DeleteRegistryValue(HKEY reg_root,
                                      const base::string16& key_path,
                                      REGSAM wow64_access,
                                      const base::string16& value_name) {
  RegKey key;
  LONG result = key.Open(reg_root, key_path.c_str(),
                         KEY_SET_VALUE | wow64_access);
  if (result == ERROR_SUCCESS)
    result = key.DeleteValue(value_name.c_str());
  if (result != ERROR_SUCCESS && result != ERROR_FILE_NOT_FOUND) {
    LOG(ERROR) << "Failed to delete registry value: " << value_name
               << " error: " << result;
    return false;
  }
  return true;
}

// static
InstallUtil::ConditionalDeleteResult InstallUtil::DeleteRegistryKeyIf(
    HKEY root_key,
    const base::string16& key_to_delete_path,
    const base::string16& key_to_test_path,
    const REGSAM wow64_access,
    const wchar_t* value_name,
    const RegistryValuePredicate& predicate) {
  DCHECK(root_key);
  ConditionalDeleteResult delete_result = NOT_FOUND;
  RegKey key;
  base::string16 actual_value;
  if (key.Open(root_key, key_to_test_path.c_str(),
               KEY_QUERY_VALUE | wow64_access) == ERROR_SUCCESS &&
      key.ReadValue(value_name, &actual_value) == ERROR_SUCCESS &&
      predicate.Evaluate(actual_value)) {
    key.Close();
    delete_result = DeleteRegistryKey(root_key,
                                      key_to_delete_path,
                                      wow64_access)
        ? DELETED : DELETE_FAILED;
  }
  return delete_result;
}

// static
InstallUtil::ConditionalDeleteResult InstallUtil::DeleteRegistryValueIf(
    HKEY root_key,
    const wchar_t* key_path,
    REGSAM wow64_access,
    const wchar_t* value_name,
    const RegistryValuePredicate& predicate) {
  DCHECK(root_key);
  DCHECK(key_path);
  ConditionalDeleteResult delete_result = NOT_FOUND;
  RegKey key;
  base::string16 actual_value;
  if (key.Open(root_key, key_path,
               KEY_QUERY_VALUE | KEY_SET_VALUE | wow64_access)
          == ERROR_SUCCESS &&
      key.ReadValue(value_name, &actual_value) == ERROR_SUCCESS &&
      predicate.Evaluate(actual_value)) {
    LONG result = key.DeleteValue(value_name);
    if (result != ERROR_SUCCESS) {
      LOG(ERROR) << "Failed to delete registry value: "
                 << (value_name ? value_name : L"(Default)")
                 << " error: " << result;
      delete_result = DELETE_FAILED;
    } else {
      delete_result = DELETED;
    }
  }
  return delete_result;
}

bool InstallUtil::ValueEquals::Evaluate(const base::string16& value) const {
  return value == value_to_match_;
}

// static
int InstallUtil::GetInstallReturnCode(installer::InstallStatus status) {
  switch (status) {
    case installer::FIRST_INSTALL_SUCCESS:
    case installer::INSTALL_REPAIRED:
    case installer::NEW_VERSION_UPDATED:
    case installer::IN_USE_UPDATED:
    case installer::OLD_VERSION_DOWNGRADE:
    case installer::IN_USE_DOWNGRADE:
      return 0;
    default:
      return status;
  }
}

// static
void InstallUtil::ComposeCommandLine(const base::string16& program,
                                     const base::string16& arguments,
                                     base::CommandLine* command_line) {
  *command_line =
      base::CommandLine::FromString(L"\"" + program + L"\" " + arguments);
}

void InstallUtil::AppendModeSwitch(base::CommandLine* command_line) {
  const install_static::InstallDetails& install_details =
      install_static::InstallDetails::Get();
  if (*install_details.install_switch())
    command_line->AppendSwitch(install_details.install_switch());
}

// static
base::string16 InstallUtil::GetCurrentDate() {
  static const wchar_t kDateFormat[] = L"yyyyMMdd";
  wchar_t date_str[arraysize(kDateFormat)] = {0};
  int len = GetDateFormatW(LOCALE_INVARIANT, 0, NULL, kDateFormat,
                           date_str, arraysize(date_str));
  if (len) {
    --len;  // Subtract terminating \0.
  } else {
    PLOG(DFATAL) << "GetDateFormat";
  }

  return base::string16(date_str, len);
}

// Open |path| with minimal access to obtain information about it, returning
// true and populating |file| on success.
// static
bool InstallUtil::ProgramCompare::OpenForInfo(const base::FilePath& path,
                                              base::File* file) {
  DCHECK(file);
  file->Initialize(path, base::File::FLAG_OPEN);
  return file->IsValid();
}

// Populate |info| for |file|, returning true on success.
// static
bool InstallUtil::ProgramCompare::GetInfo(const base::File& file,
                                          BY_HANDLE_FILE_INFORMATION* info) {
  DCHECK(file.IsValid());
  return GetFileInformationByHandle(file.GetPlatformFile(), info) != 0;
}

// static
base::Version InstallUtil::GetDowngradeVersion(
    bool system_install,
    const BrowserDistribution* dist) {
  DCHECK(dist);
  base::win::RegKey key;
  base::string16 downgrade_version;
  if (key.Open(system_install ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER,
               dist->GetStateKey().c_str(),
               KEY_QUERY_VALUE | KEY_WOW64_32KEY) != ERROR_SUCCESS ||
      key.ReadValue(kRegDowngradeVersion, &downgrade_version) !=
          ERROR_SUCCESS) {
    return base::Version();
  }
  return base::Version(base::UTF16ToASCII(downgrade_version));
}

// static
void InstallUtil::AddUpdateDowngradeVersionItem(
    bool system_install,
    const base::Version* current_version,
    const base::Version& new_version,
    const BrowserDistribution* dist,
    WorkItemList* list) {
  DCHECK(list);
  DCHECK(dist);
  base::Version downgrade_version = GetDowngradeVersion(system_install, dist);
  HKEY root = system_install ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
  if (!current_version ||
      (*current_version <= new_version &&
       ((!downgrade_version.IsValid() || downgrade_version <= new_version)))) {
    list->AddDeleteRegValueWorkItem(root, dist->GetStateKey(), KEY_WOW64_32KEY,
                                    kRegDowngradeVersion);
  } else if (*current_version > new_version && !downgrade_version.IsValid()) {
    list->AddSetRegValueWorkItem(
        root, dist->GetStateKey(), KEY_WOW64_32KEY, kRegDowngradeVersion,
        base::ASCIIToUTF16(current_version->GetString()), true);
  }
}

InstallUtil::ProgramCompare::ProgramCompare(const base::FilePath& path_to_match)
    : path_to_match_(path_to_match),
      file_info_() {
  DCHECK(!path_to_match_.empty());
  if (!OpenForInfo(path_to_match_, &file_)) {
    PLOG(WARNING) << "Failed opening " << path_to_match_.value()
                  << "; falling back to path string comparisons.";
  } else if (!GetInfo(file_, &file_info_)) {
    PLOG(WARNING) << "Failed getting information for "
                  << path_to_match_.value()
                  << "; falling back to path string comparisons.";
    file_.Close();
  }
}

InstallUtil::ProgramCompare::~ProgramCompare() {
}

bool InstallUtil::ProgramCompare::Evaluate(const base::string16& value) const {
  // Suss out the exe portion of the value, which is expected to be a command
  // line kinda (or exactly) like:
  // "c:\foo\bar\chrome.exe" -- "%1"
  base::FilePath program(base::CommandLine::FromString(value).GetProgram());
  if (program.empty()) {
    LOG(WARNING) << "Failed to parse an executable name from command line: \""
                 << value << "\"";
    return false;
  }

  return EvaluatePath(program);
}

bool InstallUtil::ProgramCompare::EvaluatePath(
    const base::FilePath& path) const {
  // Try the simple thing first: do the paths happen to match?
  if (base::FilePath::CompareEqualIgnoreCase(path_to_match_.value(),
                                             path.value()))
    return true;

  // If the paths don't match and we couldn't open the expected file, we've done
  // our best.
  if (!file_.IsValid())
    return false;

  // Open the program and see if it references the expected file.
  base::File file;
  BY_HANDLE_FILE_INFORMATION info = {};

  return (OpenForInfo(path, &file) &&
          GetInfo(file, &info) &&
          info.dwVolumeSerialNumber == file_info_.dwVolumeSerialNumber &&
          info.nFileIndexHigh == file_info_.nFileIndexHigh &&
          info.nFileIndexLow == file_info_.nFileIndexLow);
}
