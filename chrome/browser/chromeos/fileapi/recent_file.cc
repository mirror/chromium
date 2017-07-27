// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/fileapi/recent_file.h"

#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

namespace chromeos {

RecentFile::RecentFile(SourceType source_type,
                       const std::string& original_file_name,
                       const std::string& unique_id)
    : source_type_(source_type),
      original_file_name_(original_file_name),
      unique_id_(unique_id) {}

RecentFile::~RecentFile() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
}

}  // namespace chromeos
