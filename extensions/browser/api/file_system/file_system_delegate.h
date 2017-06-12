// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_FILE_SYSTEM_FILE_SYSTEM_DELEGATE_H_
#define EXTENSIONS_BROWSER_API_FILE_SYSTEM_FILE_SYSTEM_DELEGATE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "build/build_config.h"
#include "extensions/browser/api/file_system/saved_files_service_delegate.h"
#include "extensions/common/api/file_system.h"
#include "ui/shell_dialogs/select_file_dialog.h"

namespace base {
class FilePath;
}  // namespace base

namespace content {
class BrowserContext;
class RenderFrameHost;
class WebContents;
}  // namespace content

namespace ui {
class SelectFilePolicy;
}  // namespace ui

namespace extensions {

class Extension;

// Delegate class for embedder-specific file system access.
class FileSystemDelegate {
 public:
  using ErrorCallback = base::Callback<void(const std::string&)>;
  using FileSystemCallback =
      base::Callback<void(const std::string& id, const std::string& path)>;
  using VolumeListCallback =
      base::Callback<void(const std::vector<api::file_system::Volume>&)>;

  FileSystemDelegate();
  virtual ~FileSystemDelegate();

  virtual base::FilePath GetDefaultDirectory();

  virtual std::unique_ptr<ui::SelectFilePolicy> CreateSelectFilePolicy(
      content::WebContents* web_contents);

  virtual scoped_refptr<ui::SelectFileDialog> CreateSelectFileDialog(
      ui::SelectFileDialog::Listener* listener,
      std::unique_ptr<ui::SelectFilePolicy> policy);

  virtual void ShowSelectFileDialogForWebContents(
      scoped_refptr<ui::SelectFileDialog> dialog,
      content::WebContents* web_contents,
      ui::SelectFileDialog::Type type,
      const base::FilePath& default_path,
      const ui::SelectFileDialog::FileTypeInfo* file_types);

  virtual void ConfirmSensitiveDirectoryAccess(
      bool writable,
      const base::string16& app_name,
      content::WebContents* web_contents,
      const base::Closure& on_accept,
      const base::Closure& on_cancel);

  // Finds a string describing the accept type. On success, returns true and
  // populates |description_id|.
  virtual bool GetDescriptionIdForAcceptType(const std::string& accept_type,
                                             int* description_id);

#if defined(OS_CHROMEOS)
  virtual bool IsGrantable(content::BrowserContext* browser_context,
                           content::RenderFrameHost* render_frame_host,
                           const Extension& extension);

  // Grants or denies an extension's request for access to the named file
  // system. May prompt the user for consent.
  virtual void RequestFileSystem(content::BrowserContext* browser_context,
                                 content::RenderFrameHost* render_frame_host,
                                 const Extension& extension,
                                 std::string volume_id,
                                 bool writable,
                                 const FileSystemCallback& success_callback,
                                 const ErrorCallback& error_callback);

  // Immediately calls VolumeListCallback or ErrorCallback.
  virtual void GetVolumeList(content::BrowserContext* browser_context,
                             const VolumeListCallback& success_callback,
                             const ErrorCallback& error_callback);
#endif

  virtual std::unique_ptr<file_system_api::SavedFilesServiceDelegate>
  CreateSavedFilesServiceDelegate(content::BrowserContext* browser_context);

 private:
  DISALLOW_COPY_AND_ASSIGN(FileSystemDelegate);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_FILE_SYSTEM_FILE_SYSTEM_DELEGATE_H_
