// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/origin_manifest/origin_manifest_store.h"

#include <tuple>

#include "base/bind.h"
#include "base/logging.h"
#include "content/browser/origin_manifest/origin_manifest.h"
#include "content/browser/origin_manifest/sqlite_persistent_origin_manifest_store.h"

namespace {

content::OriginManifestStore& store_ =
    *new content::OriginManifestStore(nullptr);
}

namespace content {

OriginManifestStore::OriginManifestStore(
    SQLitePersistentOriginManifestStore* persistent)
    : persistent_store_(persistent) {}
OriginManifestStore::~OriginManifestStore() {}

// static
void OriginManifestStore::Init(
    SQLitePersistentOriginManifestStore* persistent) {
  DCHECK(persistent != nullptr);

  store_.SetPersistentStore(persistent);
  store_.persistent_store_->Load(
      base::Bind(&OriginManifestStore::OnLoaded, base::Unretained(&store_)));
}

void OriginManifestStore::OnLoaded(
    std::vector<std::unique_ptr<OriginManifest>> omanifests) {
  for (std::vector<std::unique_ptr<OriginManifest>>::iterator it =
           omanifests.begin();
       it != omanifests.end(); ++it) {
    std::string origin = it->get()->Origin();
    std::string version = it->get()->Version();
    manifests[origin] = std::make_tuple(version, std::move(*it));
  }
}

// static
OriginManifestStore& OriginManifestStore::Get() {
  return store_;
}

std::string OriginManifestStore::GetVersion(std::string origin) {
  if (manifests.find(origin) != manifests.end())
    return std::get<0>(manifests[origin]);

  return "1";
}

const OriginManifest* OriginManifestStore::GetManifest(std::string origin,
                                                       std::string version) {
  if (manifests.find(origin) != manifests.end())
    return std::get<1>(manifests[origin]).get();

  return nullptr;
}

void OriginManifestStore::Add(std::string origin,
                              std::string version,
                              std::unique_ptr<OriginManifest> originmanifest) {
  OriginManifest* om_ptr = originmanifest.get();
  manifests[origin] = std::make_tuple(version, std::move(originmanifest));
  if (persistent_store_)
    persistent_store_->AddOriginManifest(*om_ptr);
}

bool OriginManifestStore::HasManifest(std::string origin, std::string version) {
  return manifests.find(origin) != manifests.end() &&
         std::get<0>(manifests[origin]) == version;
}

void OriginManifestStore::Revoke(std::string origin) {
  if (manifests.find(origin) == manifests.end())
    return;

  OriginManifest* originmanifest = std::get<1>(manifests[origin]).get();
  originmanifest->Revoke();

  if (persistent_store_)
    persistent_store_->DeleteOriginManifest(*originmanifest);

  manifests.erase(origin);
}

void OriginManifestStore::SetPersistentStore(
    SQLitePersistentOriginManifestStore* persistent) {
  persistent_store_ = persistent;
}

const char* OriginManifestStore::GetNameForLogging() {
  return "OriginManifestStore";
}

}  // namespace content
