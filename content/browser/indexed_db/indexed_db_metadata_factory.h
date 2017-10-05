// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_METADATA_FACTORY_H_
#define CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_METADATA_FACTORY_H_

#include <stdint.h>
#include <map>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/strings/string16.h"
#include "content/common/indexed_db/indexed_db_key_path.h"
#include "third_party/leveldatabase/src/include/leveldb/status.h"

namespace content {
class LevelDBDatabase;
class LevelDBTransaction;
struct IndexedDBDatabaseMetadata;
struct IndexedDBObjectStoreMetadata;
struct IndexedDBIndexMetadata;

// Creation, reading, and common operations for IndexedDB metadata.
class CONTENT_EXPORT IndexedDBMetadataFactory {
 public:
  IndexedDBMetadataFactory();
  virtual ~IndexedDBMetadataFactory();

  // Reads the list of database names for the given origin.
  virtual leveldb::Status GetDatabaseNames(LevelDBDatabase* db,
                                           const std::string& origin_identifier,
                                           std::vector<base::string16>* names);

  // Reads in metadata for the database and all object stores & indices.
  // Note: the database name is not populated in |metadata|.
  virtual leveldb::Status ReadFullDatabaseMetadata(
      LevelDBDatabase* db,
      const std::string& origin_identifier,
      const base::string16& name,
      IndexedDBDatabaseMetadata* metadata,
      bool* found);

  // Creates a new database metadata entry and writes it to disk.
  virtual leveldb::Status CreateAndWriteDatabaseMetadata(
      LevelDBDatabase* database,
      const std::string& origin_identifier,
      base::string16 name,
      int64_t version,
      IndexedDBDatabaseMetadata* metadata);

  // Updates the database version to |version|.
  virtual void UpdateDatabaseVersion(LevelDBTransaction* transaction,
                                     int64_t row_id,
                                     int64_t version,
                                     IndexedDBDatabaseMetadata* metadata);

  // Reads only the database id.
  virtual leveldb::Status ReadDatabaseId(LevelDBDatabase* db,
                                         const std::string& origin_identifier,
                                         const base::string16& name,
                                         int64_t* id,
                                         bool* found);

  // Reads all object stores and indexes for the given |database_id| into
  // |object_stores|.
  virtual leveldb::Status ReadObjectStoresAndIndexes(
      LevelDBDatabase* database,
      int64_t database_id,
      std::map<int64_t, IndexedDBObjectStoreMetadata>* object_stores);

  // Creates a new object store metadata entry and writes it to disk.
  virtual leveldb::Status CreateAndWriteObjectStore(
      LevelDBTransaction* transaction,
      int64_t database_id,
      int64_t object_store_id,
      base::string16 name,
      IndexedDBKeyPath key_path,
      bool auto_increment,
      IndexedDBObjectStoreMetadata* metadata);

  // Renames the given object store and writes it to disk.
  virtual leveldb::Status RenameObjectStore(
      LevelDBTransaction* transaction,
      int64_t database_id,
      base::string16 new_name,
      base::string16* old_name,
      IndexedDBObjectStoreMetadata* metadata);

  // Deletes the given object store metadata from disk (but not any data entries
  // or blobs in the object store).
  virtual leveldb::Status DeleteObjectStoreMetadata(
      LevelDBTransaction* transaction,
      int64_t database_id,
      const IndexedDBObjectStoreMetadata& object_store);

  // Reads all indexes for the given database and object store in |indexes|.
  virtual leveldb::Status ReadIndexes(
      LevelDBDatabase* database,
      int64_t database_id,
      int64_t object_store_id,
      std::map<int64_t, IndexedDBIndexMetadata>* indexes);

  // Creates a new index metadata and writes it to disk.
  virtual leveldb::Status CreateAndWriteIndex(LevelDBTransaction* transaction,
                                              int64_t database_id,
                                              int64_t object_store_id,
                                              int64_t index_id,
                                              base::string16 name,
                                              IndexedDBKeyPath key_path,
                                              bool is_unique,
                                              bool is_multi_entry,
                                              IndexedDBIndexMetadata* metadata);

  // Renames the given index and writes it to disk.
  virtual leveldb::Status RenameIndex(LevelDBTransaction* transaction,
                                      int64_t database_id,
                                      int64_t object_store_id,
                                      base::string16 new_name,
                                      base::string16* old_name,
                                      IndexedDBIndexMetadata* metadata);

  // Deletes the index metadata from disk (but not any index entries).
  virtual leveldb::Status DeleteIndexMetadata(
      LevelDBTransaction* transaction,
      int64_t database_id,
      int64_t object_store_id,
      const IndexedDBIndexMetadata& metadata);

 private:
  DISALLOW_COPY_AND_ASSIGN(IndexedDBMetadataFactory);
};

}  // namespace content
#endif  // CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_METADATA_FACTORY_H_
