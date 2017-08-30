// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/origin_manifest/origin_manifest_storage.h"

#include <tuple>

#include "base/logging.h"
#include "content/browser/origin_manifest/origin_manifest.h"

namespace {
std::map<std::string,
         std::tuple<std::string, std::unique_ptr<content::OriginManifest>>>&
    manifests = *new std::map<
        std::string,
        std::tuple<std::string, std::unique_ptr<content::OriginManifest>>>();
}

namespace content {
class OriginManifest;

OriginManifestStorage::OriginManifestStorage() {}
OriginManifestStorage::~OriginManifestStorage() {}

std::string OriginManifestStorage::GetVersion(std::string origin) {
  if (manifests.find(origin) != manifests.end()) {
    return std::get<0>(manifests[origin]);
  }
  return "1";
}

const OriginManifest* OriginManifestStorage::GetManifest(std::string origin,
                                                         std::string version) {
  if (manifests.find(origin) != manifests.end()) {
    return std::get<1>(manifests[origin]).get();
  }
  return nullptr;
}

void OriginManifestStorage::Add(
    std::string origin,
    std::string version,
    std::unique_ptr<OriginManifest> originmanifest) {
  manifests[origin] = std::make_tuple(version, std::move(originmanifest));
}

bool OriginManifestStorage::HasManifest(std::string origin,
                                        std::string version) {
  return manifests.find(origin) != manifests.end() &&
         std::get<0>(manifests[origin]) == version;
}

void OriginManifestStorage::Revoke(std::string origin) {
  if (manifests.find(origin) != manifests.end()) {
    std::get<1>(manifests[origin])->Revoke();
    manifests.erase(origin);
  }
}

const char* OriginManifestStorage::GetNameForLogging() {
  return "OriginManifestStorage";
}

}  // namespace content
