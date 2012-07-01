// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/fileapi/isolated_mount_point_provider.h"

#include <string>

#include "base/bind.h"
#include "base/file_path.h"
#include "base/logging.h"
#include "base/message_loop_proxy.h"
#include "base/sequenced_task_runner.h"
#include "webkit/blob/local_file_stream_reader.h"
#include "webkit/fileapi/file_system_callback_dispatcher.h"
#include "webkit/fileapi/file_system_context.h"
#include "webkit/fileapi/file_system_file_stream_reader.h"
#include "webkit/fileapi/file_system_operation.h"
#include "webkit/fileapi/file_system_types.h"
#include "webkit/fileapi/file_system_util.h"
#include "webkit/fileapi/isolated_context.h"
#include "webkit/fileapi/isolated_file_util.h"
#include "webkit/fileapi/local_file_stream_writer.h"
#include "webkit/fileapi/native_file_util.h"

namespace fileapi {

namespace {

IsolatedContext* isolated_context() {
  return IsolatedContext::GetInstance();
}

FilePath GetPathFromURL(const FileSystemURL& url) {
  if (!url.is_valid() || url.type() != kFileSystemTypeIsolated)
    return FilePath();
  std::string fsid;
  FilePath path;
  if (!isolated_context()->CrackIsolatedPath(url.path(), &fsid, NULL, &path))
    return FilePath();
  return path;
}

}  // namespace

IsolatedMountPointProvider::IsolatedMountPointProvider()
    : isolated_file_util_(new IsolatedFileUtil()) {
}

IsolatedMountPointProvider::~IsolatedMountPointProvider() {
}

void IsolatedMountPointProvider::ValidateFileSystemRoot(
    const GURL& origin_url,
    FileSystemType type,
    bool create,
    const ValidateFileSystemCallback& callback) {
  // We never allow opening a new isolated FileSystem via usual OpenFileSystem.
  base::MessageLoopProxy::current()->PostTask(
      FROM_HERE,
      base::Bind(callback, base::PLATFORM_FILE_ERROR_SECURITY));
}

FilePath IsolatedMountPointProvider::GetFileSystemRootPathOnFileThread(
    const GURL& origin_url,
    FileSystemType type,
    const FilePath& virtual_path,
    bool create) {
  if (create || type != kFileSystemTypeIsolated)
    return FilePath();
  std::string fsid;
  FilePath root, path;
  if (!isolated_context()->CrackIsolatedPath(virtual_path, &fsid, &root, &path))
    return FilePath();
  return root;
}

bool IsolatedMountPointProvider::IsAccessAllowed(
    const GURL& origin_url, FileSystemType type, const FilePath& virtual_path) {
  if (type != fileapi::kFileSystemTypeIsolated)
    return false;

  std::string filesystem_id;
  FilePath root, path;
  return isolated_context()->CrackIsolatedPath(
      virtual_path, &filesystem_id, &root, &path);
}

bool IsolatedMountPointProvider::IsRestrictedFileName(
    const FilePath& filename) const {
  return false;
}

FileSystemFileUtil* IsolatedMountPointProvider::GetFileUtil() {
  return isolated_file_util_.get();
}

FilePath IsolatedMountPointProvider::GetPathForPermissionsCheck(
    const FilePath& virtual_path) const {
  std::string fsid;
  FilePath root, path;
  if (!isolated_context()->CrackIsolatedPath(virtual_path, &fsid, &root, &path))
    return FilePath();
  return path;
}

FileSystemOperationInterface*
IsolatedMountPointProvider::CreateFileSystemOperation(
    const FileSystemURL& url,
    FileSystemContext* context) const {
  return new FileSystemOperation(context);
}

webkit_blob::FileStreamReader*
IsolatedMountPointProvider::CreateFileStreamReader(
    const FileSystemURL& url,
    int64 offset,
    FileSystemContext* context) const {
  FilePath path = GetPathFromURL(url);
  return path.empty() ? NULL : new webkit_blob::LocalFileStreamReader(
      context->file_task_runner(), path, offset, base::Time());
}

FileStreamWriter* IsolatedMountPointProvider::CreateFileStreamWriter(
    const FileSystemURL& url,
    int64 offset,
    FileSystemContext* context) const {
  FilePath path = GetPathFromURL(url);
  return path.empty() ? NULL : new LocalFileStreamWriter(path, offset);
}

FileSystemQuotaUtil* IsolatedMountPointProvider::GetQuotaUtil() {
  // No quota support.
  return NULL;
}

}  // namespace fileapi
