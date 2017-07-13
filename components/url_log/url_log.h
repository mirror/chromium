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
#include "components/url_log/url_log_export.h"
#include "url/gurl.h"

namespace url_log {

// The switch for the file path that UrlLog writes to.
const char kLogUrlLog[] = "log-url-log";

// UrlLog writes URLs in NetworkDelegate::OnBeforeURLRequest to a file
// specified via a command-line switch. The write happens on the current thread
// and is immediately flushed to dish; even if the browser
// subsequently crashes, the output file will contain all fetched URLs.
// This is critical for ClusterFuzz whose goal is to discover crashes and be
// able to reproduce them.
//
// This is not thread-safe. However, NetworkDelegate::OnBeforeURLRequest is
// thread-safe.
//
// Each line in the text file is a URL.
class URL_LOG_EXPORT UrlLog {
 public:
  // Get the UrlLog singleton instance and write a URL to the file (specified
  // via a command-line switch).
  static void AddEntry(const GURL& url);

  // Get the UrlLog singleton instance. Other classes shouldn't call this
  // method directly. Instead, use AddEntry.
  static UrlLog* GetInstance();

  // Write the url to the file. Other classes shouldn't call this method.
  // Instead, use AddEntry.
  void Log(const GURL& url);

  // The constructor should only be used in tests. Use AddEntry.
  UrlLog();
  ~UrlLog();

 private:
  friend struct base::LeakySingletonTraits<UrlLog>;

  base::ScopedFILE file_;
  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(UrlLog);
};

}  // namespace url_log

#endif  // COMPONENTS_URL_LOG_URL_LOG_H_
