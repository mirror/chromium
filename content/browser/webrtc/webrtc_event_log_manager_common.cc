// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webrtc/webrtc_event_log_manager_common.h"

#include <limits>

namespace content {

bool LogFileWriter::WriteToLogFile(LogFilesMap::iterator it,
                                   const std::string& output) {
  DCHECK_LE(output.length(),
            static_cast<size_t>(std::numeric_limits<int>::max()));

  // Observe the file size limit, if any. Note that base::File's interface does
  // not allow writing more than numeric_limits<int>::max() bytes at a time.
  int output_len = static_cast<int>(output.length());  // DCHECKed above.
  LogFile& log_file = it->second;
  if (log_file.max_file_size_bytes != kWebRtcEventLogManagerUnlimitedFileSize) {
    DCHECK_LT(log_file.file_size_bytes, log_file.max_file_size_bytes);
    if (log_file.file_size_bytes + output.length() < log_file.file_size_bytes ||
        log_file.file_size_bytes + output.length() >
            log_file.max_file_size_bytes) {
      output_len = log_file.max_file_size_bytes - log_file.file_size_bytes;
    }
  }

  int written = log_file.file.WriteAtCurrentPos(output.c_str(), output_len);
  if (written < 0 || written != output_len) {  // Error
    LOG(WARNING) << "WebRTC event log output couldn't be written to the "
                    "locally stored file in its entirety.";
    CloseLogFile(it);
    return false;
  }

  log_file.file_size_bytes += static_cast<size_t>(written);
  if (log_file.max_file_size_bytes != kWebRtcEventLogManagerUnlimitedFileSize) {
    DCHECK_LE(log_file.file_size_bytes, log_file.max_file_size_bytes);
    if (log_file.file_size_bytes >= log_file.max_file_size_bytes) {
      CloseLogFile(it);
    }
  }

  // Truncated output due to exceeding the maximum is reported as an error - the
  // caller is interested to know that not all of its output was written,
  // regardless of the reason.
  return (static_cast<size_t>(written) == output.length());
}

}  // namespace content
