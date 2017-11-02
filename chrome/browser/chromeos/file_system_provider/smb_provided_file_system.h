// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_FILE_SYSTEM_PROVIDER_SMB_PROVIDED_FILE_SYSTEM_H_
#define CHROME_BROWSER_CHROMEOS_FILE_SYSTEM_PROVIDER_SMB_PROVIDED_FILE_SYSTEM_H_

#include <stdint.h>

#include <map>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/files/file.h"
#include "base/macros.h"
#include "base/memory/linked_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "chrome/browser/chromeos/file_system_provider/abort_callback.h"
#include "chrome/browser/chromeos/file_system_provider/provided_file_system_info.h"
#include "chrome/browser/chromeos/file_system_provider/provided_file_system_interface.h"
#include "chrome/browser/chromeos/file_system_provider/watcher.h"
#include "storage/browser/fileapi/async_file_util.h"
#include "storage/browser/fileapi/watcher_manager.h"
#include "url/gurl.h"

namespace net {
class IOBuffer;
}  // namespace net

namespace chromeos {
namespace file_system_provider {

class RequestManager;

// Fake provided file system implementation. Does not communicate with target
// extensions. Used for unit tests.
class SmbProvidedFileSystem : public ProvidedFileSystemInterface {
 public:
  explicit SmbProvidedFileSystem(
      const ProvidedFileSystemInfo& file_system_info);
  ~SmbProvidedFileSystem() override;

  // ProvidedFileSystemInterface overrides.
  AbortCallback RequestUnmount(
      const storage::AsyncFileUtil::StatusCallback& callback) override;

  AbortCallback GetMetadata(
      const base::FilePath& entry_path,
      ProvidedFileSystemInterface::MetadataFieldMask fields,
      const ProvidedFileSystemInterface::GetMetadataCallback& callback)
      override;

  AbortCallback GetActions(const std::vector<base::FilePath>& entry_paths,
                           const GetActionsCallback& callback) override;

  AbortCallback ExecuteAction(
      const std::vector<base::FilePath>& entry_paths,
      const std::string& action_id,
      const storage::AsyncFileUtil::StatusCallback& callback) override;

  AbortCallback ReadDirectory(
      const base::FilePath& directory_path,
      const storage::AsyncFileUtil::ReadDirectoryCallback& callback) override;

  AbortCallback OpenFile(const base::FilePath& file_path,
                         OpenFileMode mode,
                         const OpenFileCallback& callback) override;

  AbortCallback CloseFile(
      int file_handle,
      const storage::AsyncFileUtil::StatusCallback& callback) override;

  AbortCallback ReadFile(int file_handle,
                         net::IOBuffer* buffer,
                         int64_t offset,
                         int length,
                         const ReadChunkReceivedCallback& callback) override;

  AbortCallback CreateDirectory(
      const base::FilePath& directory_path,
      bool recursive,
      const storage::AsyncFileUtil::StatusCallback& callback) override;

  AbortCallback CreateFile(
      const base::FilePath& file_path,
      const storage::AsyncFileUtil::StatusCallback& callback) override;

  AbortCallback DeleteEntry(
      const base::FilePath& entry_path,
      bool recursive,
      const storage::AsyncFileUtil::StatusCallback& callback) override;

  AbortCallback CopyEntry(
      const base::FilePath& source_path,
      const base::FilePath& target_path,
      const storage::AsyncFileUtil::StatusCallback& callback) override;

  AbortCallback MoveEntry(
      const base::FilePath& source_path,
      const base::FilePath& target_path,
      const storage::AsyncFileUtil::StatusCallback& callback) override;

  AbortCallback Truncate(
      const base::FilePath& file_path,
      int64_t length,
      const storage::AsyncFileUtil::StatusCallback& callback) override;

  AbortCallback WriteFile(
      int file_handle,
      net::IOBuffer* buffer,
      int64_t offset,
      int length,
      const storage::AsyncFileUtil::StatusCallback& callback) override;

  AbortCallback AddWatcher(
      const GURL& origin,
      const base::FilePath& entry_path,
      bool recursive,
      bool persistent,
      const storage::AsyncFileUtil::StatusCallback& callback,
      const storage::WatcherManager::NotificationCallback&
          notification_callback) override;

  void RemoveWatcher(
      const GURL& origin,
      const base::FilePath& entry_path,
      bool recursive,
      const storage::AsyncFileUtil::StatusCallback& callback) override;

  const ProvidedFileSystemInfo& GetFileSystemInfo() const override;

  RequestManager* GetRequestManager() override;

  Watchers* GetWatchers() override;

  const OpenedFiles& GetOpenedFiles() const override;

  void AddObserver(ProvidedFileSystemObserver* observer) override;

  void RemoveObserver(ProvidedFileSystemObserver* observer) override;

  void Notify(const base::FilePath& entry_path,
              bool recursive,
              storage::WatcherManager::ChangeType change_type,
              std::unique_ptr<ProvidedFileSystemObserver::Changes> changes,
              const std::string& tag,
              const storage::AsyncFileUtil::StatusCallback& callback) override;

  void Configure(
      const storage::AsyncFileUtil::StatusCallback& callback) override;

  base::WeakPtr<ProvidedFileSystemInterface> GetWeakPtr() override;

 private:
  ProvidedFileSystemInfo file_system_info_;
  OpenedFiles opened_files_;
  storage::AsyncFileUtil::EntryList entry_list_;
  Watchers watchers_;

  base::WeakPtrFactory<SmbProvidedFileSystem> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(SmbProvidedFileSystem);
};

}  // namespace file_system_provider
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_FILE_SYSTEM_PROVIDER_SMB_PROVIDED_FILE_SYSTEM_H_
