// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBRTC_WEBRTC_RTC_EVENT_LOG_MANAGER_H_
#define CONTENT_BROWSER_WEBRTC_WEBRTC_RTC_EVENT_LOG_MANAGER_H_

#include <map>
#include <set>
#include <string>
#include <utility>  // TODO(eladalon): !!!

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/lazy_instance.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/sequenced_task_runner.h"
#include "content/common/content_export.h"

namespace content {

// This is a singleton class running in the browser UI thread.
// It is in charge of writing RTC event logs to temporary files, then uploading
// those files to remote servers.
// TODO(eladalon): Currently, only write to files. Add the code that also
// uploads those files.
class CONTENT_EXPORT RtcEventLogManager {
 public:
  static RtcEventLogManager* GetInstance();

  RtcEventLogManager();
  ~RtcEventLogManager();

  // TODO(eladalon): !!! Rename
  void RtcEventLogOpen(int render_process_id, int lid);
  void RtcEventLogClose(int render_process_id, int lid);

  void OnRtcEventLogWrite(int render_process_id,
                          int lid,
                          const std::string& output);

 private:
  friend struct base::LazyInstanceTraitsBase<RtcEventLogManager>;

  void ProcessPendingLogsPath();

  void OpenLogFile(int render_process_id, int lid);
  void WriteToLogFile(int render_process_id,
                      int lid,
                      const std::string& output);

  const base::FilePath logs_base_path_;

  // TODO(eladalon): !!! Guarantee that these are only accessed on task_runner_.

  // TODO(eladalon): !!! Why doesn't this work?
//  // For a given Chrome session, this is a unique key for PeerConnections.
//  // It's not, however, unique between sessions (after Chrome is restarted).
//  struct PeerConnectionKey {
//    int render_process_id;
//    int lid;
//  };
  using PeerConnectionKey = std::pair<int, int>;

  // Note: Ownership of the files is held in this map.
  std::map<PeerConnectionKey, base::File> pc_to_log_file_mapping_;

  // TODO(eladalon): For now, this prevents (the already unlikely) collision of
  // new filenames with names of previously created logs (from previous
  // sessions). It will be removed by later CLs which will examination of
  // pending logs, uploading them to the server, and deleting them.
  std::set<std::string> unavailable_filenames_;

  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  base::WeakPtrFactory<RtcEventLogManager> weak_ptr_factory_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEBRTC_WEBRTC_RTC_EVENT_LOG_MANAGER_H_
