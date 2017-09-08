// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/child/database_util.h"

#include "content/common/database_messages.h"
#include "content/common/web_database.mojom.h"
#include "ipc/ipc_sync_message_filter.h"
#include "third_party/WebKit/public/platform/WebSecurityOrigin.h"
#include "third_party/WebKit/public/platform/WebString.h"
#include "third_party/sqlite/sqlite3.h"

using blink::Platform;
using blink::WebSecurityOrigin;
using blink::WebString;

namespace content {

Platform::FileHandle DatabaseUtil::DatabaseOpenFile(
    const WebString& vfs_file_name,
    int desired_flags,
    content::mojom::WebDatabaseHost& web_database_host) {
  base::File file;
  if (web_database_host.OpenFile(vfs_file_name.Utf16(), desired_flags, &file)) {
    return file.TakePlatformFile();
  }
  return base::kInvalidPlatformFile;
}

int DatabaseUtil::DatabaseDeleteFile(
    const WebString& vfs_file_name,
    bool sync_dir,
    content::mojom::WebDatabaseHost& web_database_host) {
  int rv = SQLITE_IOERR_DELETE;
  web_database_host.DeleteFile(vfs_file_name.Utf16(), sync_dir, &rv);
  return rv;
}

long DatabaseUtil::DatabaseGetFileAttributes(
    const WebString& vfs_file_name,
    content::mojom::WebDatabaseHost& web_database_host) {
  int32_t rv = -1;
  web_database_host.GetFileAttributes(vfs_file_name.Utf16(), &rv);
  return rv;
}

long long DatabaseUtil::DatabaseGetFileSize(
    const WebString& vfs_file_name,
    content::mojom::WebDatabaseHost& web_database_host) {
  int64_t rv = 0LL;
  web_database_host.GetFileSize(vfs_file_name.Utf16(), &rv);
  return rv;
}

long long DatabaseUtil::DatabaseGetSpaceAvailable(
    const WebSecurityOrigin& origin,
    content::mojom::WebDatabaseHost& web_database_host) {
  int64_t rv = 0LL;
  web_database_host.GetSpaceAvailable(origin, &rv);
  return rv;
}

bool DatabaseUtil::DatabaseSetFileSize(
    const WebString& vfs_file_name,
    int64_t size,
    content::mojom::WebDatabaseHost& web_database_host) {
  bool rv = false;
  web_database_host.SetFileSize(vfs_file_name.Utf16(), size, &rv);
  return rv;
}

}  // namespace content
