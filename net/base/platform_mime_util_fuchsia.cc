// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/platform_mime_util.h"

#include <string>

#include "base/logging.h"
#include "build/build_config.h"

namespace net {

namespace {

struct MimeToExt {
  const char* mime_type;
  const char* extension;
};

const struct MimeToExt kMimeToExtMap[] = {
    {"application/pdf", "pdf"}, {"application/x-tar", "tar"},
    {"application/zip", "zip"}, {"audio/mpeg", "mp3"},
    {"image/gif", "gif"},       {"image/jpeg", "jpg"},
    {"image/png", "png"},       {"text/html", "html"},
    {"video/mp4", "mp4"},       {"video/mpeg", "mpg"},
    {"text/plain", "txt"},      {"text/x-sh", "sh"},
};

}  // namespace

bool PlatformMimeUtil::GetPlatformMimeTypeFromExtension(
    const base::FilePath::StringType& ext,
    std::string* result) const {
  for (size_t x = 0; x < arraysize(kMimeToExtMap); x++) {
    if (kMimeToExtMap[x].extension == ext) {
      *result = kMimeToExtMap[x].mime_type;
      return true;
    }
  }

  // TODO(fuchsia): Integrate with MIME DB when Fuchsia provides an API.
  return false;
}

bool PlatformMimeUtil::GetPreferredExtensionForMimeType(
    const std::string& mime_type,
    base::FilePath::StringType* ext) const {
  for (size_t x = 0; x < arraysize(kMimeToExtMap); x++) {
    if (kMimeToExtMap[x].mime_type == mime_type) {
      *ext = kMimeToExtMap[x].extension;
      return true;
    }
  }

  // TODO(fuchsia): Integrate with MIME DB when Fuchsia provides an API.
  return false;
}

void PlatformMimeUtil::GetPlatformExtensionsForMimeType(
    const std::string& mime_type,
    std::unordered_set<base::FilePath::StringType>* extensions) const {
  base::FilePath::StringType ext;
  if (GetPreferredExtensionForMimeType(mime_type, &ext))
    extensions->insert(ext);
}

}  // namespace net
