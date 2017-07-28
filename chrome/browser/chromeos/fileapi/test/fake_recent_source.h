// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_FILEAPI_TEST_FAKE_RECENT_SOURCE_H_
#define CHROME_BROWSER_CHROMEOS_FILEAPI_TEST_FAKE_RECENT_SOURCE_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "chrome/browser/chromeos/fileapi/recent_file.h"
#include "chrome/browser/chromeos/fileapi/recent_source.h"
#include "storage/browser/fileapi/file_system_url.h"

namespace chromeos {

// Fake implementation of RecentSource that returns a canned set of files.
//
// All member functions must be called on the IO thread.
class FakeRecentSource : public RecentSource {
 public:
  explicit FakeRecentSource(RecentFile::SourceType source_type);
  ~FakeRecentSource() override;

  // Add a file to the canned set.
  void AddFile(const std::string& original_file_name,
               const std::string& unique_id,
               int64_t size);

  // RecentSource overrides:
  void GetRecentFiles(RecentContext context,
                      GetRecentFilesCallback callback) override;

 private:
  class FakeRecentFile;

  const RecentFile::SourceType source_type_;
  std::vector<std::unique_ptr<FakeRecentFile>> canned_files_;

  DISALLOW_COPY_AND_ASSIGN(FakeRecentSource);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_FILEAPI_TEST_FAKE_RECENT_SOURCE_H_
