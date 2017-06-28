// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


#include <stdio.h>

#include "net/log/log_urls_to_file.h"

namespace net_log {

LogUrlsToFile* LogUrlsToFile::GetInstance() {
  return base::Singleton<::net_log::LogUrlsToFile,
      base::StaticMemorySingletonTraits<LogUrlsToFile>>::get();
}

void LogUrlsToFile::SetPath(const base::FilePath path) {
  DCHECK(!file_.get());
  // Much like logging.h, bypass threading restrictions by using fopen
  // directly. Have to write on a thread that's shutdown to handle events on
  // shutdown properly, and posting events to another thread as they occur
  // would result in an unbounded buffer size, so not much can be gained by
  // doing this on another thread.  It's only used when debugging Chrome, so
  // performance is not a big concern.
#if defined(OS_WIN)
  file_.reset(_wfopen(path.value().c_str(), L"w"));
#elif defined(OS_POSIX)
  file_.reset(fopen(path.value().c_str(), "w"));
#endif
}

LogUrlsToFile::LogUrlsToFile() {}
LogUrlsToFile::~LogUrlsToFile() {}

void LogUrlsToFile::LogURL(const GURL& url) {
  fprintf(file_.get(), "%s\n", url.possibly_invalid_spec().c_str());
  fflush(file_.get());
}

}  // namespace net_log

