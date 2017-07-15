// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/url_log/url_log.h"

#include <stdio.h>
#include <string>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/memory/ptr_util.h"
#include "build/build_config.h"
#include "url/gurl.h"

namespace url_log {

UrlLog::UrlLog() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(kLogUrlLog)) {
    base::FilePath path =
        base::CommandLine::ForCurrentProcess()->GetSwitchValuePath(kLogUrlLog);
// Much like logging.h, bypass threading restrictions by using fopen
// directly. It's only used when debugging Chrome, so performance is not a big
// concern.
#if defined(OS_WIN)
    file_.reset(_wfopen(path.value().c_str(), L"wb"));
#elif defined(OS_POSIX)
    file_.reset(fopen(path.value().c_str(), "w"));
#endif
    setbuf(file_.get(), nullptr);
  }
}

UrlLog::~UrlLog() {}

void UrlLog::Log(const GURL& url) {
  UrlLog::GetInstance()->AppendToFile(url);
}

UrlLog* UrlLog::GetInstance() {
  return base::Singleton<UrlLog, base::LeakySingletonTraits<UrlLog>>::get();
}

void UrlLog::AppendToFile(const GURL& url) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!file_.get()) {
    return;
  }

  fprintf(file_.get(), "%s\n", url.possibly_invalid_spec().c_str());
}

}  // namespace url_log
