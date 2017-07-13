// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/url_log/url_log.h"

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/threading/thread_restrictions.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_base.h"
#include "net/base/filename_util.h"
#include "url/gurl.h"

namespace url_log {

class UrlLogBrowserTestBase {
 public:
  UrlLogBrowserTestBase();
  ~UrlLogBrowserTestBase();

  void SetUp();

  void SetUpCommandLine(base::CommandLine* command_line);

  GURL GetUrl(const base::FilePath& path, const std::string& child);

  void WriteUrlLogTest();

 protected:
  base::ScopedTempDir temp_dir_;
  base::FilePath log_path_;
  base::FilePath test_data_dir_;

 private:
  DISALLOW_COPY_AND_ASSIGN(UrlLogBrowserTestBase);
};

}  // namespace

