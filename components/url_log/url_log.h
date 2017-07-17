// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_URL_LOG_URL_LOG_H_
#define COMPONENTS_URL_LOG_URL_LOG_H_

#include "base/files/scoped_file.h"
#include "base/macros.h"
#include "base/memory/singleton.h"
#include "base/sequence_checker.h"
#include "components/url_log/url_log_export.h"

class GURL;

namespace url_log {

// Command line switch to specify the destination file path where a
// newline-separated list of URLs are written to.  This switch is intended only
// for testing, and may have performance consequences. It is used as:
// --log-url-log=LOGPATH
const char kLogUrlLog[] = "log-url-log";

// UrlLog is a singleton for appending URLs to a log file specified via
// --log-url-log. Writes happen immediately on the calling thread. Even if the
// browser subsequently crashes, the output file will contain all fetched URLs.
// Usage example:
//
// UrlLog::Log(url);
//
// This is critical for ClusterFuzz whose goal is to discover crashes and be
// able to reproduce them.
class URL_LOG_EXPORT UrlLog {
 public:
  // The constructor should only be used in tests. Use Log.
  UrlLog();
  ~UrlLog();

  // Gets the UrlLog singleton instance and write a URL to the file (specified
  // via a command-line switch).
  static void Log(const GURL& url);

  // Gets the UrlLog singleton instance. Other classes shouldn't call this
  // method directly. Instead, use Log.
  static UrlLog* GetInstance();

  // Writes the url to the file. Other classes shouldn't call this method.
  // Instead, use Log.
  void AppendToFile(const GURL& url);

 private:
  friend struct base::DefaultSingletonTraits<UrlLog>;

  base::ScopedFILE file_;
  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(UrlLog);
};

}  // namespace url_log

#endif  // COMPONENTS_URL_LOG_URL_LOG_H_
