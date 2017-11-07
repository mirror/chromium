// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/webrtc/webrtc_logs_directory_access_util.h"

#include "chrome/browser/media/webrtc/webrtc_log_list.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/child_process_security_policy.h"
#include "extensions/browser/granted_file_entry.h"
#include "storage/browser/fileapi/isolated_context.h"

namespace webrtc_logs_directory_access_util {

// Creates a new directory entry and grants |render_process_id| read-only access
// the WebRTC Logs directory for |profile|. This registers a new file system.
extensions::GrantedFileEntry GetLogsDirectoryEntry(Profile* profile,
                                                   int render_process_id) {
  extensions::GrantedFileEntry result;

  base::FilePath path =
      WebRtcLogList::GetWebRtcLogDirectoryForProfile(profile->GetPath());

  storage::IsolatedContext* isolated_context =
      storage::IsolatedContext::GetInstance();
  DCHECK(isolated_context);

  result.filesystem_id = isolated_context->RegisterFileSystemForPath(
      storage::kFileSystemTypeNativeLocal, std::string(), path,
      &result.registered_name);

  // Only granting read-only access to reduce contention with webrtcLogging APIs
  // that modify files in that folder.
  content::ChildProcessSecurityPolicy* policy =
      content::ChildProcessSecurityPolicy::GetInstance();
  policy->GrantReadFileSystem(render_process_id, result.filesystem_id);

  // Not setting result.id, since it's not needed by
  // fileEntryBindingUtil.getBindDirectoryEntryCallback to create a
  // DirectoryEntry.
  return result;
}

}  // namespace webrtc_logs_directory_access_util
