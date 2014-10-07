// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_FAKE_BACKING_STORE_H_
#define CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_FAKE_BACKING_STORE_H_

#include <vector>

#include "content/browser/indexed_db/indexed_db_backing_store.h"

namespace base {
class SequencedTaskRunner;
}

namespace content {

class IndexedDBFactory;

class IndexedDBFakeBackingStore : public IndexedDBBackingStore {
 public:
  IndexedDBFakeBackingStore();
  IndexedDBFakeBackingStore(IndexedDBFactory* factory,
                            base::SequencedTaskRunner* task_runner);
  virtual std::vector<base::string16> GetDatabaseNames(leveldb::Status* s)
      override;
  virtual leveldb::Status GetIDBDatabaseMetaData(const base::string16& name,
                                                 IndexedDBDatabaseMetadata*,
                                                 bool* found) override;
  virtual leveldb::Status CreateIDBDatabaseMetaData(
      const base::string16& name,
      const base::string16& version,
      int64 int_version,
      int64* row_id) override;
  virtual bool UpdateIDBDatabaseIntVersion(Transaction*,
                                           int64 row_id,
                                           int64 version) override;
  virtual leveldb::Status DeleteDatabase(const base::string16& name) override;

  virtual leveldb::Status CreateObjectStore(Transaction*,
                                            int64 database_id,
                                            int64 object_store_id,
                                            const base::string16& name,
                                            const IndexedDBKeyPath&,
                                            bool auto_increment) override;

  virtual leveldb::Status DeleteObjectStore(Transaction* transaction,
                                            int64 database_id,
                                            int64 object_store_id) override;

  virtual leveldb::Status PutRecord(
      IndexedDBBackingStore::Transaction* transaction,
      int64 database_id,
      int64 object_store_id,
      const IndexedDBKey& key,
      IndexedDBValue* value,
      ScopedVector<storage::BlobDataHandle>* handles,
      RecordIdentifier* record) override;

  virtual leveldb::Status ClearObjectStore(Transaction*,
                                           int64 database_id,
                                           int64 object_store_id) override;
  virtual leveldb::Status DeleteRecord(Transaction*,
                                       int64 database_id,
                                       int64 object_store_id,
                                       const RecordIdentifier&) override;
  virtual leveldb::Status GetKeyGeneratorCurrentNumber(Transaction*,
                                                       int64 database_id,
                                                       int64 object_store_id,
                                                       int64* current_number)
      override;
  virtual leveldb::Status MaybeUpdateKeyGeneratorCurrentNumber(
      Transaction*,
      int64 database_id,
      int64 object_store_id,
      int64 new_number,
      bool check_current) override;
  virtual leveldb::Status KeyExistsInObjectStore(
      Transaction*,
      int64 database_id,
      int64 object_store_id,
      const IndexedDBKey&,
      RecordIdentifier* found_record_identifier,
      bool* found) override;

  virtual leveldb::Status CreateIndex(Transaction*,
                                      int64 database_id,
                                      int64 object_store_id,
                                      int64 index_id,
                                      const base::string16& name,
                                      const IndexedDBKeyPath&,
                                      bool is_unique,
                                      bool is_multi_entry) override;
  virtual leveldb::Status DeleteIndex(Transaction*,
                                      int64 database_id,
                                      int64 object_store_id,
                                      int64 index_id) override;
  virtual leveldb::Status PutIndexDataForRecord(Transaction*,
                                                int64 database_id,
                                                int64 object_store_id,
                                                int64 index_id,
                                                const IndexedDBKey&,
                                                const RecordIdentifier&)
      override;
  virtual void ReportBlobUnused(int64 database_id, int64 blob_key) override;
  virtual scoped_ptr<Cursor> OpenObjectStoreKeyCursor(
      Transaction* transaction,
      int64 database_id,
      int64 object_store_id,
      const IndexedDBKeyRange& key_range,
      blink::WebIDBCursorDirection,
      leveldb::Status*) override;
  virtual scoped_ptr<Cursor> OpenObjectStoreCursor(
      Transaction* transaction,
      int64 database_id,
      int64 object_store_id,
      const IndexedDBKeyRange& key_range,
      blink::WebIDBCursorDirection,
      leveldb::Status*) override;
  virtual scoped_ptr<Cursor> OpenIndexKeyCursor(
      Transaction* transaction,
      int64 database_id,
      int64 object_store_id,
      int64 index_id,
      const IndexedDBKeyRange& key_range,
      blink::WebIDBCursorDirection,
      leveldb::Status*) override;
  virtual scoped_ptr<Cursor> OpenIndexCursor(Transaction* transaction,
                                             int64 database_id,
                                             int64 object_store_id,
                                             int64 index_id,
                                             const IndexedDBKeyRange& key_range,
                                             blink::WebIDBCursorDirection,
                                             leveldb::Status*) override;

  class FakeTransaction : public IndexedDBBackingStore::Transaction {
   public:
    explicit FakeTransaction(leveldb::Status phase_two_result);
    virtual void Begin() override;
    virtual leveldb::Status CommitPhaseOne(
        scoped_refptr<BlobWriteCallback>) override;
    virtual leveldb::Status CommitPhaseTwo() override;
    virtual void Rollback() override;

   private:
    leveldb::Status result_;

    DISALLOW_COPY_AND_ASSIGN(FakeTransaction);
  };

 protected:
  friend class base::RefCounted<IndexedDBFakeBackingStore>;
  virtual ~IndexedDBFakeBackingStore();

 private:
  DISALLOW_COPY_AND_ASSIGN(IndexedDBFakeBackingStore);
};

}  // namespace content

#endif  // CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_FAKE_BACKING_STORE_H_
