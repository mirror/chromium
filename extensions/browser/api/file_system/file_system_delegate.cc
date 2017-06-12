// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/file_system/file_system_delegate.h"

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "ui/shell_dialogs/select_file_policy.h"

namespace extensions {

#if defined(OS_CHROMEOS)
namespace {
const char kNotSupportedOnCurrentPlatformError[] =
    "Operation not supported on the current platform.";
}  // namespace
#endif

FileSystemDelegate::FileSystemDelegate() {}

FileSystemDelegate::~FileSystemDelegate() {}

base::FilePath FileSystemDelegate::GetDefaultDirectory() {
  NOTIMPLEMENTED();
  return base::FilePath();
}

std::unique_ptr<ui::SelectFilePolicy>
FileSystemDelegate::CreateSelectFilePolicy(content::WebContents* web_contents) {
  NOTIMPLEMENTED();
  return nullptr;
}

scoped_refptr<ui::SelectFileDialog> FileSystemDelegate::CreateSelectFileDialog(
    ui::SelectFileDialog::Listener* listener,
    std::unique_ptr<ui::SelectFilePolicy> policy) {
  NOTIMPLEMENTED();
  return nullptr;
}

void FileSystemDelegate::ShowSelectFileDialogForWebContents(
    scoped_refptr<ui::SelectFileDialog> dialog,
    content::WebContents* web_contents,
    ui::SelectFileDialog::Type type,
    const base::FilePath& default_path,
    const ui::SelectFileDialog::FileTypeInfo* file_types) {
  NOTIMPLEMENTED();
}

void FileSystemDelegate::ConfirmSensitiveDirectoryAccess(
    bool writable,
    const base::string16& app_name,
    content::WebContents* web_contents,
    const base::Closure& on_accept,
    const base::Closure& on_cancel) {
  NOTIMPLEMENTED();
}

bool FileSystemDelegate::GetDescriptionIdForAcceptType(
    const std::string& accept_type,
    int* description_id) {
  NOTIMPLEMENTED();
  return false;
}

#if defined(OS_CHROMEOS)
bool FileSystemDelegate::IsGrantable(
    content::BrowserContext* browser_context,
    content::RenderFrameHost* render_frame_host,
    const Extension& extension) {
  NOTIMPLEMENTED();
  return false;
}

void FileSystemDelegate::RequestFileSystem(
    content::BrowserContext* browser_context,
    content::RenderFrameHost* render_frame_host,
    const Extension& extension,
    std::string volume_id,
    bool writable,
    const FileSystemCallback& success_callback,
    const ErrorCallback& error_callback) {
  NOTIMPLEMENTED();
  error_callback.Run(kNotSupportedOnCurrentPlatformError);
}

void FileSystemDelegate::GetVolumeList(
    content::BrowserContext* browser_context,
    const VolumeListCallback& success_callback,
    const ErrorCallback& error_callback) {
  NOTIMPLEMENTED();
  error_callback.Run(kNotSupportedOnCurrentPlatformError);
}
#endif

std::unique_ptr<file_system_api::SavedFilesServiceDelegate>
FileSystemDelegate::CreateSavedFilesServiceDelegate(
    content::BrowserContext* browser_context) {
  NOTIMPLEMENTED();
  return nullptr;
}

}  // namespace extensions
