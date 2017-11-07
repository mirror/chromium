// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/samba_client/samba_file_system.h"

namespace chromeos {
namespace samba_client {

using file_system_provider::AbortCallback;

SambaFileSystem::SambaFileSystem(
    const file_system_provider::ProvidedFileSystemInfo& file_system_info)
    : file_system_info_(file_system_info), weak_ptr_factory_(this) {}

SambaFileSystem::~SambaFileSystem() {}

AbortCallback SambaFileSystem::RequestUnmount(
    const storage::AsyncFileUtil::StatusCallback& callback) {
  NOTIMPLEMENTED();
  return AbortCallback();
}

AbortCallback SambaFileSystem::GetMetadata(
    const base::FilePath& entry_path,
    ProvidedFileSystemInterface::MetadataFieldMask fields,
    const ProvidedFileSystemInterface::GetMetadataCallback& callback) {
  NOTIMPLEMENTED();
  return AbortCallback();
}

AbortCallback SambaFileSystem::GetActions(
    const std::vector<base::FilePath>& entry_paths,
    const GetActionsCallback& callback) {
  NOTIMPLEMENTED();
  return AbortCallback();
}

AbortCallback SambaFileSystem::ExecuteAction(
    const std::vector<base::FilePath>& entry_paths,
    const std::string& action_id,
    const storage::AsyncFileUtil::StatusCallback& callback) {
  NOTIMPLEMENTED();
  return AbortCallback();
}

AbortCallback SambaFileSystem::ReadDirectory(
    const base::FilePath& directory_path,
    const storage::AsyncFileUtil::ReadDirectoryCallback& callback) {
  NOTIMPLEMENTED();
  return AbortCallback();
}

AbortCallback SambaFileSystem::OpenFile(const base::FilePath& file_path,
                                        file_system_provider::OpenFileMode mode,
                                        const OpenFileCallback& callback) {
  NOTIMPLEMENTED();
  return AbortCallback();
}

AbortCallback SambaFileSystem::CloseFile(
    int file_handle,
    const storage::AsyncFileUtil::StatusCallback& callback) {
  NOTIMPLEMENTED();
  return AbortCallback();
}

AbortCallback SambaFileSystem::ReadFile(
    int file_handle,
    net::IOBuffer* buffer,
    int64_t offset,
    int length,
    const ReadChunkReceivedCallback& callback) {
  NOTIMPLEMENTED();
  return AbortCallback();
}

AbortCallback SambaFileSystem::CreateDirectory(
    const base::FilePath& directory_path,
    bool recursive,
    const storage::AsyncFileUtil::StatusCallback& callback) {
  NOTIMPLEMENTED();
  return AbortCallback();
}

AbortCallback SambaFileSystem::CreateFile(
    const base::FilePath& file_path,
    const storage::AsyncFileUtil::StatusCallback& callback) {
  NOTIMPLEMENTED();
  return AbortCallback();
}

AbortCallback SambaFileSystem::DeleteEntry(
    const base::FilePath& entry_path,
    bool recursive,
    const storage::AsyncFileUtil::StatusCallback& callback) {
  NOTIMPLEMENTED();
  return AbortCallback();
}

AbortCallback SambaFileSystem::CopyEntry(
    const base::FilePath& source_path,
    const base::FilePath& target_path,
    const storage::AsyncFileUtil::StatusCallback& callback) {
  NOTIMPLEMENTED();
  return AbortCallback();
}

AbortCallback SambaFileSystem::MoveEntry(
    const base::FilePath& source_path,
    const base::FilePath& target_path,
    const storage::AsyncFileUtil::StatusCallback& callback) {
  NOTIMPLEMENTED();
  return AbortCallback();
}

AbortCallback SambaFileSystem::Truncate(
    const base::FilePath& file_path,
    int64_t length,
    const storage::AsyncFileUtil::StatusCallback& callback) {
  NOTIMPLEMENTED();
  return AbortCallback();
}

AbortCallback SambaFileSystem::WriteFile(
    int file_handle,
    net::IOBuffer* buffer,
    int64_t offset,
    int length,
    const storage::AsyncFileUtil::StatusCallback& callback) {
  NOTIMPLEMENTED();
  return AbortCallback();
}

AbortCallback SambaFileSystem::AddWatcher(
    const GURL& origin,
    const base::FilePath& entry_path,
    bool recursive,
    bool persistent,
    const storage::AsyncFileUtil::StatusCallback& callback,
    const storage::WatcherManager::NotificationCallback&
        notification_callback) {
  NOTIMPLEMENTED();
  return AbortCallback();
}

void SambaFileSystem::RemoveWatcher(
    const GURL& origin,
    const base::FilePath& entry_path,
    bool recursive,
    const storage::AsyncFileUtil::StatusCallback& callback) {
  NOTIMPLEMENTED();
}

const file_system_provider::ProvidedFileSystemInfo&
SambaFileSystem::GetFileSystemInfo() const {
  return file_system_info_;
}

file_system_provider::RequestManager* SambaFileSystem::GetRequestManager() {
  NOTIMPLEMENTED();
  return NULL;
}

file_system_provider::Watchers* SambaFileSystem::GetWatchers() {
  NOTIMPLEMENTED();
  return &watchers_;
}

const file_system_provider::OpenedFiles& SambaFileSystem::GetOpenedFiles()
    const {
  NOTIMPLEMENTED();
  return opened_files_;
}

void SambaFileSystem::AddObserver(
    file_system_provider::ProvidedFileSystemObserver* observer) {
  NOTIMPLEMENTED();
}

void SambaFileSystem::RemoveObserver(
    file_system_provider::ProvidedFileSystemObserver* observer) {
  NOTIMPLEMENTED();
}

void SambaFileSystem::SambaFileSystem::Notify(
    const base::FilePath& entry_path,
    bool recursive,
    storage::WatcherManager::ChangeType change_type,
    std::unique_ptr<file_system_provider::ProvidedFileSystemObserver::Changes>
        changes,
    const std::string& tag,
    const storage::AsyncFileUtil::StatusCallback& callback) {
  NOTIMPLEMENTED();
}

void SambaFileSystem::Configure(
    const storage::AsyncFileUtil::StatusCallback& callback) {
  NOTIMPLEMENTED();
}

base::WeakPtr<file_system_provider::ProvidedFileSystemInterface>
SambaFileSystem::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

}  // namespace samba_client
}  // namespace chromeos
