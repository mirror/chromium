// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_INDEXED_DB_FAKE_INDEXED_DB_METADATA_FACTORY_H_
#define CONTENT_BROWSER_INDEXED_DB_FAKE_INDEXED_DB_METADATA_FACTORY_H_

#include <stdint.h>
#include <map>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/strings/string16.h"
#include "content/browser/indexed_db/indexed_db_metadata_factory.h"
#include "content/common/indexed_db/indexed_db_key_path.h"
#include "third_party/leveldatabase/src/include/leveldb/status.h"

namespace content {
class LevelDBDatabase;
class LevelDBTransaction;
struct IndexedDBDatabaseMetadata;
struct IndexedDBObjectStoreMetadata;
struct IndexedDBIndexMetadata;

class FakeIndexedDBMetadataFactory : public IndexedDBMetadataFactory {
 public:
  FakeIndexedDBMetadataFactory();
  ~FakeIndexedDBMetadataFactory() override;

  leveldb::Status GetDatabaseNames(LevelDBDatabase* db,
                                   const std::string& origin_identifier,
                                   std::vector<base::string16>* names) override;

  leveldb::Status ReadFullDatabaseMetadata(LevelDBDatabase* db,
                                           const std::string& origin_identifier,
                                           const base::string16& name,
                                           IndexedDBDatabaseMetadata* metadata,
                                           bool* found) override;
  leveldb::Status CreateAndWriteDatabaseMetadata(
      LevelDBDatabase* database,
      const std::string& origin_identifier,
      base::string16 name,
      int64_t version,
      IndexedDBDatabaseMetadata* metadata) override;

  void UpdateDatabaseVersion(LevelDBTransaction* transaction,
                             int64_t row_id,
                             int64_t version,
                             IndexedDBDatabaseMetadata* metadata) override;

  leveldb::Status ReadDatabaseId(LevelDBDatabase* db,
                                 const std::string& origin_identifier,
                                 const base::string16& name,
                                 int64_t* id,
                                 bool* found) override;

  leveldb::Status ReadObjectStoresAndIndexes(
      LevelDBDatabase* database,
      int64_t database_id,
      std::map<int64_t, IndexedDBObjectStoreMetadata>* object_stores) override;

  leveldb::Status CreateAndWriteObjectStore(
      LevelDBTransaction* transaction,
      int64_t database_id,
      int64_t object_store_id,
      base::string16 name,
      IndexedDBKeyPath key_path,
      bool auto_increment,
      IndexedDBObjectStoreMetadata* metadata) override;

  leveldb::Status RenameObjectStore(
      LevelDBTransaction* transaction,
      int64_t database_id,
      base::string16 new_name,
      base::string16* old_name,
      IndexedDBObjectStoreMetadata* metadata) override;

  leveldb::Status DeleteObjectStoreMetadata(
      LevelDBTransaction* transaction,
      int64_t database_id,
      const IndexedDBObjectStoreMetadata& object_store) override;

  leveldb::Status ReadIndexes(
      LevelDBDatabase* database,
      int64_t database_id,
      int64_t object_store_id,
      std::map<int64_t, IndexedDBIndexMetadata>* indexes) override;

  leveldb::Status CreateAndWriteIndex(
      LevelDBTransaction* transaction,
      int64_t database_id,
      int64_t object_store_id,
      int64_t index_id,
      base::string16 name,
      IndexedDBKeyPath key_path,
      bool is_unique,
      bool is_multi_entry,
      IndexedDBIndexMetadata* metadata) override;

  leveldb::Status RenameIndex(LevelDBTransaction* transaction,
                              int64_t database_id,
                              int64_t object_store_id,
                              base::string16 new_name,
                              base::string16* old_name,
                              IndexedDBIndexMetadata* metadata) override;

  leveldb::Status DeleteIndexMetadata(
      LevelDBTransaction* transaction,
      int64_t database_id,
      int64_t object_store_id,
      const IndexedDBIndexMetadata& metadata) override;
};

}  // namespace content
#endif  // CONTENT_BROWSER_INDEXED_DB_FAKE_INDEXED_DB_METADATA_FACTORY_H_
