// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/updater/update_install_shim.h"

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

UpdateInstallShim::UpdateInstallShim(
    std::string extension_id,
    const base::FilePath& extension_root,
    UpdateInstallShimCallback update_install_callback)
    : extension_id_(extension_id),
      extension_root_(extension_root),
      update_install_callback_(std::move(update_install_callback)) {}

void UpdateInstallShim::OnUpdateError(int error) {
  VLOG(1) << "OnUpdateError (" << extension_id_ << ") " << error;
}

void UpdateInstallShim::Install(const base::FilePath& unpack_path,
                                const std::string& public_key,
                                UpdateClientCallback update_client_callback) {
  auto ui_thread = content::BrowserThread::GetTaskRunnerForThread(
      content::BrowserThread::UI);
  DCHECK(ui_thread);
  if (base::PathExists(unpack_path)) {
    ui_thread->PostTask(
        FROM_HERE,
        base::BindOnce(&UpdateInstallShim::RunInstallCallbackOnUIThread, this,
                       unpack_path, public_key,
                       std::move(update_client_callback)));
    return;
  }
  ui_thread->PostTask(FROM_HERE,
                      base::BindOnce(std::move(update_client_callback),
                                     Result(InstallError::GENERIC_ERROR)));
}

bool UpdateInstallShim::GetInstalledFile(const std::string& file,
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

bool UpdateInstallShim::Uninstall() {
  NOTREACHED();
  return false;
}

UpdateInstallShim::~UpdateInstallShim() {}

void UpdateInstallShim::RunInstallCallbackOnUIThread(
    const base::FilePath& unpacked_dir,
    const std::string& public_key,
    UpdateClientCallback update_client_callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(!update_install_callback_.is_null());

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(update_install_callback_),
                                extension_id_, public_key, unpacked_dir,
                                std::move(update_client_callback)));
}

}  // namespace extensions
