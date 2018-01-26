// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_FILENAME_GENERATION_FILENAME_GENERATION_H_
#define COMPONENTS_FILENAME_GENERATION_FILENAME_GENERATION_H_

#include <string>

#include "base/files/file_path.h"
#include "base/strings/string16.h"

class GURL;

namespace filename_generation {

const base::FilePath::CharType* ExtensionForMimeType(
    const std::string& contents_mime_type);

base::FilePath EnsureHtmlExtension(const base::FilePath& name);

base::FilePath EnsureMimeExtension(const base::FilePath& name,
                                   const std::string& contents_mime_type);

base::FilePath GenerateUniqueFilename(const base::FilePath& save_dir,
                                      const base::string16& title,
                                      const GURL& url);
base::FilePath GenerateFilename(const base::string16& title,
                                const GURL& url,
                                bool can_save_as_complete,
                                std::string contents_mime_type);

}  // namespace filename_generation

#endif  // COMPONENTS_FILENAME_GENERATION_FILENAME_GENERATION_H_
