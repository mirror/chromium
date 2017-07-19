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
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "content/browser/indexed_db/indexed_db_leveldb_coding.h"
#include "content/common/indexed_db/indexed_db_metadata.h"
#include "third_party/leveldatabase/src/include/leveldb/status.h"
#include "third_party/leveldatabase/src/include/leveldb/write_batch.h"

namespace leveldb {
class DB;
class Iterator;
}  // namespace leveldb

namespace base {
class SequencedTaskRunner;
}

namespace content {
class IndexedDBBackingStore;

// Crawls the backing store looking for index tombstones. These occur when
// the indexed fields of rows are modified, and stay around if the user doesn't
// do a cursor iteration of the database.
//
// Pause: The crawler pauses first before crawling.
// Round: Crawling is split up into rounds to we don't monopolize the thread and
//        allow stopping.
// Lifetime: Expects to be outlived by the IndexedDBBackingStore.
//
// See go/idb-tombstone-crawler for more information.
class IndexedDBTombstoneCrawler {
 public:
  enum class Mode {
    // Gathers statistics and doesn't modify the database.
    STATISTICS,
    // Deletes the tombstones that are encountered.
    DELETION,
    // Just pauses.
    NOOP
  };

  enum class State {
    PAUSED,
    CRAWLING,
    DONE_NEW_CONNECTION,
    DONE_TIMOUT,
    DONE_REACHED_MAX,
    DONE_ERROR,
    DONE_COMPLETE
  };

  IndexedDBTombstoneCrawler(Mode mode,
                            size_t round_iterations,
                            size_t max_iterations,
                            base::TimeDelta pause_time,
                            base::TimeDelta maximum_crawl_time);
  ~IndexedDBTombstoneCrawler();

  // We assume that the backing store outlives this object, and that the
  // internal leveldb database isn't swapped.
  // |done| is called on success or error. It is not called if this class is
  // destructed before it is complete.
  void Start(IndexedDBBackingStore* backing_store, base::OnceClosure done);

  // Signals the crawler to stop because a new connection to the database was
  // created.
  void StopForNewConnection();

  State state() { return state_; }

  // If the crawler has been stopped.
  bool IsStopped();

 private:
  using ObjectStoreMetadataMap =
      std::map<int64_t, IndexedDBObjectStoreMetadata>;
  using IndexMetadataMap = std::map<int64_t, IndexedDBIndexMetadata>;

  struct CrawlState {
    CrawlState();
    ~CrawlState();

    size_t metadata_idx = 0;
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

  enum class CrawlStatus { PENDING, STOPPED };

  void ClearAndCallDone();

  void OnPauseDone();

  void OnTimeoutDone();

  void FlushDeletions();

  leveldb::Status ScanMetadata();

  void CrawlLoop();

  bool ShouldContinueIteration(CrawlStatus* crawl_status,
                               leveldb::Status* leveldb_status,
                               size_t* round_iters);

  CrawlStatus DoCrawl(leveldb::Status* status);

  bool start_called_ = false;
  const Mode mode_;
  State state_ = State::PAUSED;
  size_t num_iterations_ = 0;
  const size_t max_round_iterations_;
  const size_t max_iterations_;

  const base::TimeDelta pause_time_;
  base::OneShotTimer pause_timer_;
  const base::TimeDelta crawl_deadline_time_;
  base::OneShotTimer crawl_deadline_timer_;
  base::OnceClosure done_;

  // We assume our IndexedDBBackingStore (and inner DB) outlives us.
  IndexedDBBackingStore* backing_store_ = nullptr;
  leveldb::DB* database_ = nullptr;
  leveldb::WriteBatch curr_deletion_batch_;

  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  std::vector<IndexedDBDatabaseMetadata> database_metadata_;
  std::unique_ptr<leveldb::Iterator> iterator_;

  CrawlState crawl_state_;
  CrawlMetrics metrics_;

  base::WeakPtrFactory<IndexedDBTombstoneCrawler> ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(IndexedDBTombstoneCrawler);
};

}  // namespace content
#endif  // CONTENT_BROWSER_INDEXED_DB_INDEXED_DB_TOMBSTONE_CRAWLER_H_
