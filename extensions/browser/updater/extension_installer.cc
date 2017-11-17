// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/updater/extension_installer.h"

#include <utility>

#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/update_client/update_client_errors.h"
#include "content/public/browser/browser_thread.h"

namespace extensions {

namespace {
using InstallError = update_client::InstallError;
using Result = update_client::CrxInstaller::Result;
}  // namespace

ExtensionInstaller::ExtensionInstaller(
    std::string extension_id,
    const base::FilePath& extension_root,
    ExtensionInstallerCallback extension_installer_callback)
    : extension_id_(extension_id),
      extension_root_(extension_root),
      extension_installer_callback_(std::move(extension_installer_callback)) {}

void ExtensionInstaller::OnUpdateError(int error) {
  VLOG(1) << "OnUpdateError (" << extension_id_ << ") " << error;
}

void ExtensionInstaller::Install(const base::FilePath& unpack_path,
                                 const std::string& public_key,
                                 UpdateClientCallback update_client_callback) {
  auto ui_thread = content::BrowserThread::GetTaskRunnerForThread(
      content::BrowserThread::UI);
  DCHECK(ui_thread);
  if (base::PathExists(unpack_path)) {
    ui_thread->PostTask(
        FROM_HERE,
        base::BindOnce(&ExtensionInstaller::RunInstallCallbackOnUIThread, this,
                       unpack_path, public_key,
                       std::move(update_client_callback)));
    return;
  }
  ui_thread->PostTask(FROM_HERE,
                      base::BindOnce(std::move(update_client_callback),
                                     Result(InstallError::GENERIC_ERROR)));
}

bool ExtensionInstaller::GetInstalledFile(const std::string& file,
                                          base::FilePath* installed_file) {
  base::FilePath relative_path = base::FilePath::FromUTF8Unsafe(file);
  if (relative_path.IsAbsolute() || relative_path.ReferencesParent())
    return false;
  *installed_file = extension_root_.Append(relative_path);
  if (!extension_root_.IsParent(*installed_file) ||
      !base::PathExists(*installed_file)) {
    VLOG(1) << "GetInstalledFile failed to find " << installed_file->value();
    installed_file->clear();
    return false;
  }
  return true;
}

bool ExtensionInstaller::Uninstall() {
  NOTREACHED();
  return false;
}

ExtensionInstaller::~ExtensionInstaller() {}

void ExtensionInstaller::RunInstallCallbackOnUIThread(
    const base::FilePath& unpacked_dir,
    const std::string& public_key,
    UpdateClientCallback update_client_callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(!extension_installer_callback_.is_null());

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(extension_installer_callback_),
                                extension_id_, public_key, unpacked_dir,
                                std::move(update_client_callback)));
}

}  // namespace extensions
