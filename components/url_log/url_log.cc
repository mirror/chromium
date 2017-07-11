// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>

#include "base/command_line.h"
#include "build/build_config.h"
#include "components/url_log/url_log.h"

namespace url_log {

UrlLog* UrlLog::GetInstance() {
  return base::Singleton<UrlLog, base::LeakySingletonTraits<UrlLog>>::get();
}

void UrlLog::AddEntry(const GURL& url) {
  UrlLog::GetInstance()->Log(url);
}

UrlLog::UrlLog() {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(kLogUrlLog)) {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    base::FilePath path =
        base::CommandLine::ForCurrentProcess()->GetSwitchValuePath(kLogUrlLog);
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
}

UrlLog::~UrlLog() {}

void UrlLog::Log(const GURL& url) {
  if (!file_.get()) {
    return;
  }
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  fprintf(file_.get(), "%s\n", url.possibly_invalid_spec().c_str());
  fflush(file_.get());
}

}  // namespace url_log
