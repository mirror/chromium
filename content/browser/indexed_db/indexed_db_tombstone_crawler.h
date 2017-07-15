// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_TOMBSTONE_CRAWLER_H_
#define CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_TOMBSTONE_CRAWLER_H_

#include <memory>

#include "base/callback.h"
#include "base/feature_list.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/time/clock.h"
#include "base/time/time.h"
#include "content/browser/indexed_db/indexed_db_leveldb_coding.h"
#include "content/common/indexed_db/indexed_db_metadata.h"
#include "third_party/leveldatabase/src/include/leveldb/status.h"
#include "third_party/leveldatabase/src/include/leveldb/write_batch.h"

namespace leveldb {
class DB;
class Iterator;
class Snapshot;
}  // namespace leveldb

namespace base {
class SequencedTaskRunner;
}

namespace content {
class IndexedDBBackingStore;

// Runs until complete or until destroyed.
class IndexedDBTombstoneCrawler {
 public:
  enum class Mode { STATISTICS, DELETION };

  IndexedDBTombstoneCrawler(Mode mode,
                            base::TimeDelta max_crawl_time,
                            std::unique_ptr<base::Clock> clock,
                            base::OnceClosure on_complete);
  ~IndexedDBTombstoneCrawler();

  base::WeakPtr<IndexedDBTombstoneCrawler> StartAndDestroy(
      leveldb::DB* database,
      scoped_refptr<IndexedDBBackingStore> backing_store);

 private:
  using ObjectStoreMetadataMap =
      std::map<int64_t, IndexedDBObjectStoreMetadata>;
  using IndexMetadataMap = std::map<int64_t, IndexedDBIndexMetadata>;

  void RecordIteration(leveldb::Status status);

  struct CrawlState {
    CrawlState();
    ~CrawlState();

    size_t database = 0;
    base::Optional<ObjectStoreMetadataMap::const_iterator> object_store_it;
    base::Optional<IndexMetadataMap::const_iterator> index_it;
    base::Optional<IndexDataKey> curr_index_key;
  };

  struct CrawlMetrics {
    int invalid_index_values = 0;
    int errors_reading_exists_table = 0;
    int invalid_exists_values = 0;

    int total_tombstones = 0;
    uint64_t total_tombstones_size = 0;
  };

  enum class CrawlStatus {
    LEVELDB_ERROR,
    MORE_ENTRIES,
    REACHED_MAX,
    REACHED_TIME,
    DONE
  };

  bool ShouldStopAfterIteration(CrawlStatus* crawl_status,
                                leveldb::Status* leveldb_status,
                                size_t* round_iters);

  void FlushDeletions();

  void CrawlOrExitAndDestruct();

  CrawlStatus DoCrawl(leveldb::Status* status);

  Mode mode_;
  size_t iterations_ = 0;
  size_t max_iterations_per_crawl_ = 10000;
  size_t max_iterations_ = 10000000;

  scoped_refptr<IndexedDBBackingStore> backing_store_;
  const leveldb::Snapshot* snapshot_ = nullptr;
  leveldb::DB* database_ = nullptr;
  leveldb::WriteBatch curr_deletion_batch_;

  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  std::vector<IndexedDBDatabaseMetadata> database_metadata_;
  std::unique_ptr<leveldb::Iterator> iterator_;
  base::Time end_time_;
  std::unique_ptr<base::Clock> clock_;
  base::OnceClosure on_complete_;

  bool started_ = false;
  CrawlState crawl_state_;
  CrawlMetrics metrics_;

  base::WeakPtrFactory<IndexedDBTombstoneCrawler> ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(IndexedDBTombstoneCrawler);
};

}  // namespace content
#endif  // CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_TOMBSTONE_CRAWLER_H_
