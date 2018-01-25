// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_RESUME_MODE_H_
#define CONTENT_PUBLIC_BROWSER_RESUME_MODE_H_

// The means by which the download was resumed.
// Used by DownloadItemImpl and UKM metrics.
namespace content {

enum CONTENT_EXPORT ResumeMode {
  RESUME_MODE_INVALID = 0,
  RESUME_MODE_IMMEDIATE_CONTINUE,
  RESUME_MODE_IMMEDIATE_RESTART,
  RESUME_MODE_USER_CONTINUE,
  RESUME_MODE_USER_RESTART
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_RESUME_MODE_H_
