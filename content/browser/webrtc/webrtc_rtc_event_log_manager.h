// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBRTC_WEBRTC_RTC_EVENT_LOG_MANAGER_H_
#define CONTENT_BROWSER_WEBRTC_WEBRTC_RTC_EVENT_LOG_MANAGER_H_

#include "content/common/content_export.h"

namespace content {

// This is a singleton class running in the browser UI thread.
// It is in charge of writing RTC event logs to temporary files, then uploading
// those files to remote servers.
class CONTENT_EXPORT RtcEventLogManager {
 public:
  static RtcEventLogManager* GetInstance();

 private:
  friend struct base::LazyInstanceTraitsBase<RtcEventLogManager>;

  RtcEventLogManager() = default;
  ~RtcEventLogManager() = default;
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEBRTC_WEBRTC_RTC_EVENT_LOG_MANAGER_H_
