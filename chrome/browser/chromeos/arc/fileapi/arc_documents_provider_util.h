// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Utilities for ARC documents provider file system.

#ifndef CHROME_BROWSER_CHROMEOS_ARC_FILEAPI_ARC_DOCUMENTS_PROVIDER_UTIL_H_
#define CHROME_BROWSER_CHROMEOS_ARC_FILEAPI_ARC_DOCUMENTS_PROVIDER_UTIL_H_

#include <string>

#include "base/files/file_path.h"

namespace storage {
class FileSystemURL;
}  // namespace storage

namespace arc {

// The name of ARC documents provider file system mount point.
extern const char kDocumentsProviderMountPointName[];

// The path of ARC documents provider file system mount point.
extern const base::FilePath::CharType kDocumentsProviderMountPointPath[];

// Parses a FileSystemURL pointing to ARC documents provider file system.
// On success, true is returned. All arguments must not be nullptr.
bool ParseDocumentsProviderUrl(const storage::FileSystemURL& url,
                               std::string* authority,
                               std::string* root_document_id,
                               base::FilePath* path);

}  // namespace arc

#endif  // CHROME_BROWSER_CHROMEOS_ARC_FILEAPI_ARC_DOCUMENTS_PROVIDER_UTIL_H_
