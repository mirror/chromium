// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_TOMBSTONE_SWEEPER_H_
#define CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_TOMBSTONE_SWEEPER_H_

#include <memory>

#include "base/callback.h"
#include "base/feature_list.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "content/browser/indexed_db/indexed_db_leveldb_coding.h"
#include "content/browser/indexed_db/indexed_db_pre_close_task_queue.h"
#include "content/common/indexed_db/indexed_db_metadata.h"
#include "third_party/leveldatabase/src/include/leveldb/status.h"
#include "third_party/leveldatabase/src/include/leveldb/write_batch.h"

namespace leveldb {
class DB;
class Iterator;
}  // namespace leveldb

namespace content {
class IndexedDBBackingStore;

template <typename T>
class WrappingIterator {
 public:
  WrappingIterator();
  WrappingIterator(const T* container, size_t start_position);
  ~WrappingIterator();
  WrappingIterator& operator=(const WrappingIterator& other) = default;

  void Next();
  bool IsValid() const { return valid_; }
  const typename T::value_type& Value() const;

 private:
  bool valid_ = false;
  size_t iterations_done_ = 0;
  typename T::const_iterator inner_;
  const T* container_ = nullptr;
};

// Sweeps the IndexedDB leveldb database looking for index tombstones. These
// occur when the indexed fields of rows are modified, and stay around if script
// doesn't do a cursor iteration of the database.
//
// Owned by the IndexedDBBackingStore.
//
// TODO(next patch) move this to a readme file for this directory.
// See https://goo.gl/YqFDaV for more information.
class IndexedDBTombstoneSweeper
    : public IndexedDBPreCloseTaskQueue::PreCloseTask {
 public:
  enum class Mode {
    // Gathers statistics and doesn't modify the database.
    STATISTICS,
    // Deletes the tombstones that are encountered.
    DELETION
  };

  IndexedDBTombstoneSweeper(Mode mode,
                            int round_iterations,
                            int max_iterations,
                            leveldb::DB* database);
  ~IndexedDBTombstoneSweeper() override;

  void SetMetadata(
      std::vector<IndexedDBDatabaseMetadata> const* metadata) override;

  void Stop(IndexedDBPreCloseTaskQueue::StopReason reason) override;

  bool RunRound() override;

 private:
  using DatabaseMetadataVector = std::vector<IndexedDBDatabaseMetadata>;
  using ObjectStoreMetadataMap =
      std::map<int64_t, IndexedDBObjectStoreMetadata>;
  using IndexMetadataMap = std::map<int64_t, IndexedDBIndexMetadata>;

  enum class Status { SWEEPING, DONE_REACHED_MAX, DONE_ERROR, DONE_COMPLETE };

  void SetStartSeedsForTesting(int64_t database_seed,
                               int64_t object_store_seed,
                               int64_t index_seed) {
    crawl_state_.start_database_seed = database_seed;
    crawl_state_.start_object_store_seed = object_store_seed;
    crawl_state_.start_index_seed = index_seed;
  }

  // Contains the current crawling state and position for the crawler.
  struct CrawlState {
    CrawlState();
    ~CrawlState();

    // Stores the random starting database seed. Not bounded.
    uint64_t start_database_seed = 0;
    base::Optional<WrappingIterator<DatabaseMetadataVector>> database_it;

    // Stores the random starting object store seed. Not bounded.
    uint64_t start_object_store_seed = 0;
    base::Optional<WrappingIterator<ObjectStoreMetadataMap>> object_store_it;

    // Stores the random starting object store seed. Not bounded.
    uint64_t start_index_seed = 0;
    base::Optional<WrappingIterator<IndexMetadataMap>> index_it;
    base::Optional<IndexDataKey> curr_index_key;
  };

  // Accumulated metrics that are reported at the end of crawling.
  struct CrawlMetrics {
    int num_invalid_index_values = 0;
    int num_errors_reading_exists_table = 0;
    int num_invalid_exists_values = 0;

    int total_tombstones = 0;
    uint64_t total_tombstones_size = 0;
  };

  // Expects one and only one optional argument to be populated. Records UMA
  // stats based on stop or completion status, as well as the mode of the
  // sweeper.
  void RecordUMAStats(
      base::Optional<IndexedDBPreCloseTaskQueue::StopReason> stop_reason,
      base::Optional<Status> status,
      const leveldb::Status& leveldb_error);

  leveldb::Status FlushDeletions();

  bool ShouldContinueIteration(Status* crawl_status,
                               leveldb::Status* leveldb_status,
                               int* round_iters);

  Status DoCrawl(leveldb::Status* status);

  // Returns if sweeper can continue iterating.
  bool IterateIndex(int64_t database_id,
                    int64_t object_store_id,
                    const IndexedDBIndexMetadata& index,
                    Status* crawl_status,
                    leveldb::Status* leveldb_status,
                    int* round_iterations);

  const Mode mode_;
  int num_iterations_ = 0;
  const int max_round_iterations_;
  const int max_iterations_;

  // We assume this database outlives us.
  leveldb::DB* database_ = nullptr;
  bool has_writes_ = false;
  leveldb::WriteBatch curr_deletion_batch_;
  base::TimeDelta total_deletion_time_;

  std::vector<IndexedDBDatabaseMetadata> const* database_metadata_ = nullptr;
  std::unique_ptr<leveldb::Iterator> iterator_;

  CrawlState crawl_state_;
  CrawlMetrics metrics_;

  base::WeakPtrFactory<IndexedDBTombstoneSweeper> ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(IndexedDBTombstoneSweeper);
};

}  // namespace content
#endif  // CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_TOMBSTONE_SWEEPER_H_
