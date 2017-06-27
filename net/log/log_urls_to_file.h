// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_LOG_URLS_TO_FILE_H_
#define COMPONENTS_LOG_URLS_TO_FILE_H_

#include <stdio.h>

#include "url/gurl.h"
#include "base/files/file_path.h"
#include "base/memory/singleton.h"


namespace net_log {

// LogUrlsToFile writes URLs in NetworkDelegate::OnBeforeURLRequest to a file
// specified // on creation. The write happens immediately. This is
// crash-resistant and critical for ClusterFuzz.
//
// This is not thread-safe. However, NetworkDelegate::OnBeforeURLRequest is
// thread-safe.
//
// Each line in the text file is a URL.
class LogUrlsToFile {
 public:
  LogUrlsToFile(const base::FilePath path);
  ~LogUrlsToFile();

  void LogURL(const GURL& url);

 private:
  base::FilePath path_;

  DISALLOW_COPY_AND_ASSIGN(LogUrlsToFile);
};

}  // namespace net

#endif  // COMPONENTS_LOG_URLS_TO_FILE_H_
