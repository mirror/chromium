// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/platform_mime_util.h"

#include <string>

#include "base/logging.h"
#include "build/build_config.h"

namespace net {

namespace {

struct MimeToExtension {
  const char* mime_type;
  const char* extension;
};

const struct MimeToExtension kMimeToExtensionMap[] = {
    {"application/pdf", "pdf"}, {"application/x-tar", "tar"},
    {"application/zip", "zip"}, {"audio/mpeg", "mp3"},
    {"image/gif", "gif"},       {"image/jpeg", "jpg"},
    {"image/png", "png"},       {"text/html", "html"},
    {"video/mp4", "mp4"},       {"video/mpeg", "mpg"},
    {"text/plain", "txt"},      {"text/x-sh", "sh"},
};

}  // namespace

bool PlatformMimeUtil::GetPlatformMimeTypeFromExtension(
    const base::FilePath::StringType& extension,
    std::string* result) const {
  for (const auto& mime_to_extension : kMimeToExtensionMap) {
    if (mime_to_extension.extension == extension) {
      *result = mime_to_extension.mime_type;
      return true;
    }
  }

  // TODO(fuchsia): Integrate with MIME DB when Fuchsia provides an API.
  return false;
}

bool PlatformMimeUtil::GetPreferredExtensionForMimeType(
    const std::string& mime_type,
    base::FilePath::StringType* extension) const {
  for (const auto& mime_to_extension : kMimeToExtensionMap) {
    if (mime_to_extension.mime_type == mime_type) {
      *extension = mime_to_extension.extension;
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
