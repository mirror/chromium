// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STORE_PREFETCH_STORE_SCHEMA_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STORE_PREFETCH_STORE_SCHEMA_H_

namespace sql {
class Connection;
}

namespace offline_pages {

// Maintains the schema of the prefetch database, ensuring creation and upgrades
// from any and all previous database versions to the latest.
// Warning: depending on the currently existing version the database might be
// razed and all data lost.
class PrefetchStoreSchema {
 public:
  static const int kCurrentVersion;
  static const int kCompatibleVersion;

  explicit PrefetchStoreSchema(sql::Connection* db);
  ~PrefetchStoreSchema();

  // Creates or upgrade the database schema as needed from information stored in
  // a metadata table. Returns |true| if the database is ready to be used,
  // |false| if creation or upgrades failed.
  bool CreateOrUpgradeIfNeeded();

  static const char* GetItemTableCreationSqlForTesting();

 private:
  sql::Connection* db_;
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_STORE_PREFETCH_STORE_SCHEMA_H_
