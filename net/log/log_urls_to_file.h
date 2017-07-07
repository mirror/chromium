// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_LOG_URLS_TO_FILE_H_
#define COMPONENTS_LOG_URLS_TO_FILE_H_

#include <stdio.h>

#include "base/files/file_path.h"
#include "base/files/scoped_file.h"
#include "base/memory/singleton.h"
#include "base/sequence_checker.h"
#include "url/gurl.h"

namespace net_log {

// LogUrlsToFile writes URLs in NetworkDelegate::OnBeforeURLRequest to a file
// specified on creation. The write happens immediately. This is
// crash-resistant and critical for ClusterFuzz.
//
// This is not thread-safe. However, NetworkDelegate::OnBeforeURLRequest is
// thread-safe.
//
// Each line in the text file is a URL.
class LogUrlsToFile {
 public:
  static LogUrlsToFile* GetInstance();
  static void LogURL(const GURL& url);
  void Log(const GURL& url);

 private:
  friend struct base::StaticMemorySingletonTraits<LogUrlsToFile>;

  LogUrlsToFile();
  ~LogUrlsToFile();

  void SetPath(const base::FilePath path);

  base::ScopedFILE file_;
  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(LogUrlsToFile);
};

}  // namespace net_log

#endif  // COMPONENTS_LOG_URLS_TO_FILE_H_
