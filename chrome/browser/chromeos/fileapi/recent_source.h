// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_FILEAPI_RECENT_SOURCE_H_
#define CHROME_BROWSER_CHROMEOS_FILEAPI_RECENT_SOURCE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "storage/browser/fileapi/file_system_url.h"

namespace chromeos {

class RecentContext;
class RecentFile;

// Interface class for a source of recent files.
//
// Recent file system retrieves recent files from several sources such as
// local directories and cloud storages. To provide files to Recent file
// system, this interface should be implemented for each source.
//
// All member functions must be called on the IO thread.
class RecentSource {
 public:
  using GetRecentFilesCallback =
      base::OnceCallback<void(std::vector<std::unique_ptr<RecentFile>>)>;

  virtual ~RecentSource();

  // Retrieves a list of recent files from this source.
  virtual void GetRecentFiles(RecentContext context,
                              GetRecentFilesCallback callback) = 0;

 protected:
  RecentSource();
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_FILEAPI_RECENT_SOURCE_H_
