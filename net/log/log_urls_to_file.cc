// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


#include <stdio.h>

#include <memory>
#include <set>
#include <utility>

#include "base/files/scoped_file.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/values.h"
#include "net/log/net_log_entry.h"
#include "net/log/net_log_util.h"
#include "net/log/log_urls_to_file.h"
#include "net/url_request/url_request_context.h"

namespace net_log {

LogUrlsToFile::LogUrlsToFile(const base::FilePath path) {
  path_ = path;

  if (!path_.empty()) {
    FILE* file;

#if defined(OS_WIN)
    file = _wfopen(path_.value().c_str(), L"w");
#elif defined(OS_POSIX)
    file = fopen(path_.value().c_str(), "w");
#endif

    fclose(file);
  }
}

LogUrlsToFile::~LogUrlsToFile() {
}

void LogUrlsToFile::LogURL(const GURL& url) {
  if (path_.empty()) { return; }

  FILE* file;

#if defined(OS_WIN)
  file = _wfopen(path_.value().c_str(), L"a");
#elif defined(OS_POSIX)
  file = fopen(path_.value().c_str(), "a");
#endif

  fprintf(file, "%s\n", url.possibly_invalid_spec().c_str());
  fflush(file);
  fclose(file);
}

}  // namespace net

