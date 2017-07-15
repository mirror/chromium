// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "url/gurl.h"

namespace url_log {

class UrlLogBrowserTestBase {
 public:
  UrlLogBrowserTestBase();

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

}  // namespace url_log
