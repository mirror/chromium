// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_FRAME_HOST_ORIGIN_MANIFEST_ORIGIN_MANIFEST_STORAGE_H_
#define CONTENT_BROWSER_FRAME_HOST_ORIGIN_MANIFEST_ORIGIN_MANIFEST_STORAGE_H_

#include <map>
#include <string>

#include "content/common/content_export.h"

namespace content {
class OriginManifest;
}

namespace content {

// TODO The Origin Manifests should be persistantly and stored cleared with
//   cookie deletions. The static nature of the class functions and the
//   manifest map makes me shiver. But hey, for a quick-and-dirty prototype it
//   should be fine... hopefully.
class CONTENT_EXPORT OriginManifestStorage {
 public:
  virtual ~OriginManifestStorage();

  static std::string GetVersion(std::string origin);
  static const OriginManifest* GetManifest(std::string origin,
                                           std::string version);
  static void Add(std::string origin,
                  std::string version,
                  std::unique_ptr<OriginManifest> originmanifest);
  static bool HasManifest(std::string origin, std::string version);
  static void Revoke(std::string origin);

  const char* GetNameForLogging();

 private:
  OriginManifestStorage();
};

}  // namespace content

#endif  // CONTENT_BROWSER_FRAME_HOST_ORIGIN_MANIFEST_ORIGIN_MANIFEST_STORAGE_H_
