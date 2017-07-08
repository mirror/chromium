// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_URL_LOG_URL_LOG_H_
#define COMPONENTS_URL_LOG_URL_LOG_H_

#include <stdio.h>

#include "base/files/file_path.h"
#include "base/files/scoped_file.h"
#include "base/memory/singleton.h"
#include "base/sequence_checker.h"
#include "url/gurl.h"

namespace url_log {

// UrlLog writes URLs in NetworkDelegate::OnBeforeURLRequest to a file
// specified on creation. The write happens immediately. This is
// crash-resistant and critical for ClusterFuzz.
//
// This is not thread-safe. However, NetworkDelegate::OnBeforeURLRequest is
// thread-safe.
//
// Each line in the text file is a URL.
class UrlLog {
 public:
  static UrlLog* GetInstance();
  static void AddEntry(const GURL& url);
  void Log(const GURL& url);

 private:
  friend struct base::StaticMemorySingletonTraits<UrlLog>;

  UrlLog();
  ~UrlLog();

  void SetPath(const base::FilePath path);

  base::ScopedFILE file_;
  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(UrlLog);
};

}  // namespace url_log

#endif  // COMPONENTS_URL_LOG_URL_LOG_H_
