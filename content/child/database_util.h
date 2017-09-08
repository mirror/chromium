// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_CHILD_DATABASE_UTIL_H_
#define CONTENT_CHILD_DATABASE_UTIL_H_

#include <stdint.h>

#include "content/child/blink_platform_impl.h"

namespace IPC {
class SyncMessageFilter;
}

namespace content {
namespace mojom {
class WebDatabaseHost;
}

// A class of utility functions used by RendererBlinkPlatformImpl to handle
// database file accesses.
class DatabaseUtil {
 public:
  static blink::Platform::FileHandle DatabaseOpenFile(
      const blink::WebString& vfs_file_name,
      int desired_flags,
      content::mojom::WebDatabaseHost&);
  static int DatabaseDeleteFile(const blink::WebString& vfs_file_name,
                                bool sync_dir,
                                content::mojom::WebDatabaseHost&);
  static long DatabaseGetFileAttributes(const blink::WebString& vfs_file_name,
                                        content::mojom::WebDatabaseHost&);
  static long long DatabaseGetFileSize(const blink::WebString& vfs_file_name,
                                       content::mojom::WebDatabaseHost&);
  static long long DatabaseGetSpaceAvailable(
      const blink::WebSecurityOrigin& origin,
      content::mojom::WebDatabaseHost&);
  static bool DatabaseSetFileSize(const blink::WebString& vfs_file_name,
                                  int64_t size,
                                  content::mojom::WebDatabaseHost&);
};

}  // namespace content

#endif  // CONTENT_CHILD_DATABASE_UTIL_H_
