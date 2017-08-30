// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ORIGIN_MANIFEST_ORIGIN_MANIFEST_STORE_H_
#define CONTENT_BROWSER_ORIGIN_MANIFEST_ORIGIN_MANIFEST_STORE_H_

#include <map>
#include <string>

#include "base/memory/ptr_util.h"
#include "content/common/content_export.h"

namespace content {
class OriginManifest;
class OriginManifestStore;
class SQLitePersistentOriginManifestStore;
}  // namespace content

namespace content {

// TODO The Origin Manifests should be cleared with cookie deletions.
class CONTENT_EXPORT OriginManifestStore {
 public:
  typedef std::map<
      std::string,
      std::tuple<std::string, std::unique_ptr<content::OriginManifest>>>
      OriginManifestMap;

  OriginManifestStore(SQLitePersistentOriginManifestStore* persistent);
  virtual ~OriginManifestStore();

  static void Init(SQLitePersistentOriginManifestStore* persistent);
  static OriginManifestStore& Get();

  // Looks up the version identifier of the Origin Manifest currently stored for
  // |origin|. If there none can be found it returns the string "1" instead.
  std::string GetVersion(std::string origin);
  const OriginManifest* GetManifest(std::string origin, std::string version);
  void Add(std::string origin,
           std::string version,
           std::unique_ptr<OriginManifest> originmanifest);
  bool HasManifest(std::string origin, std::string version);
  void Revoke(std::string origin);

  const char* GetNameForLogging();

 private:
  void OnLoaded(std::vector<std::unique_ptr<OriginManifest>> omanifests);
  void SetPersistentStore(SQLitePersistentOriginManifestStore* persistent);

  OriginManifestMap manifests;
  SQLitePersistentOriginManifestStore* persistent_store_;
  //
};

}  // namespace content

#endif  // CONTENT_BROWSER_ORIGIN_MANIFEST_ORIGIN_MANIFEST_STORE_H_
