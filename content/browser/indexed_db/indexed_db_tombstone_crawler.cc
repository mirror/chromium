// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/indexed_db/indexed_db_tombstone_crawler.h"

#include "base/metrics/histogram_macros.h"
#include "base/sequenced_task_runner.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "content/browser/indexed_db/indexed_db_backing_store.h"
#include "third_party/leveldatabase/env_chromium.h"
#include "third_party/leveldatabase/src/include/leveldb/db.h"
#include "third_party/leveldatabase/src/include/leveldb/iterator.h"

namespace content {
namespace {

static base::StringPiece MakeStringPiece(const leveldb::Slice& s) {
  return base::StringPiece(s.data(), s.size());
}

}  // namespace

IndexedDBTombstoneCrawler::IndexedDBTombstoneCrawler(
    Mode mode,
    size_t round_iterations,
    size_t max_iterations,
    base::TimeDelta pause_time,
    base::TimeDelta maximum_crawl_time)
    : mode_(mode),
      max_round_iterations_(round_iterations),
      max_iterations_(max_iterations),
      pause_time_(pause_time),
      crawl_deadline_time_(maximum_crawl_time),
      task_runner_(base::SequencedTaskRunnerHandle::Get()),
      ptr_factory_(this) {}

IndexedDBTombstoneCrawler::~IndexedDBTombstoneCrawler() {}

IndexedDBTombstoneCrawler::CrawlState::CrawlState() = default;
IndexedDBTombstoneCrawler::CrawlState::~CrawlState() = default;

void IndexedDBTombstoneCrawler::Start(IndexedDBBackingStore* backing_store,
                                      base::OnceClosure done) {
  CHECK(!start_called_);
  done_ = std::move(done);
  start_called_ = true;
  backing_store_ = backing_store;
  database_ = backing_store->db_->db();
  pause_timer_.Start(FROM_HERE, pause_time_,
                     base::Bind(&IndexedDBTombstoneCrawler::OnPauseDone,
                                ptr_factory_.GetWeakPtr()));
}

void IndexedDBTombstoneCrawler::StopForNewConnection() {
  if (IsStopped())
    return;

  state_ = State::DONE_NEW_CONNECTION;
}

bool IndexedDBTombstoneCrawler::IsStopped() {
  switch (state_) {
    case State::PAUSED:
      return false;
    case State::CRAWLING:
      return false;
    case State::DONE_NEW_CONNECTION:
      return true;
    case State::DONE_TIMOUT:
      return true;
    case State::DONE_REACHED_MAX:
      return true;
    case State::DONE_ERROR:
      return true;
    case State::DONE_COMPLETE:
      return true;
  }
  NOTREACHED();
  return true;
}

void IndexedDBTombstoneCrawler::ClearAndCallDone() {
  iterator_.reset();
  std::move(done_).Run();
}

void IndexedDBTombstoneCrawler::OnPauseDone() {
  if (mode_ == Mode::NOOP) {
    state_ = State::DONE_COMPLETE;
    ClearAndCallDone();
    return;
  }
  leveldb::Status status = ScanMetadata();
  if (!status.ok()) {
    state_ = State::DONE_ERROR;
    ClearAndCallDone();
    return;
  }
  state_ = State::CRAWLING;
  crawl_deadline_timer_.Start(
      FROM_HERE, crawl_deadline_time_,
      base::Bind(&IndexedDBTombstoneCrawler::OnTimeoutDone,
                 ptr_factory_.GetWeakPtr()));
  CrawlLoop();
}

void IndexedDBTombstoneCrawler::OnTimeoutDone() {
  if (IsStopped())
    return;

  state_ = State::DONE_TIMOUT;
}

void IndexedDBTombstoneCrawler::FlushDeletions() {
  SCOPED_UMA_HISTOGRAM_TIMER(
      "WebCore.IndexedDB.TombstoneCrawler.DeletionCommitTime");
  LOG(ERROR) << "flushing deletions " << curr_deletion_batch_.ApproximateSize();
  leveldb::WriteOptions options;
  options.sync = true;
  database_->Write(options, &curr_deletion_batch_);
  curr_deletion_batch_.Clear();
}

leveldb::Status IndexedDBTombstoneCrawler::ScanMetadata() {
  leveldb::ReadOptions iterator_options;
  iterator_options.fill_cache = false;
  iterator_options.verify_checksums = true;
  iterator_.reset(database_->NewIterator(iterator_options));
  start_called_ = true;

  leveldb::Status status = leveldb::Status::OK();
  std::vector<base::string16> names = backing_store_->GetDatabaseNames(&status);
  if (!status.ok()) {
    LOCAL_HISTOGRAM_ENUMERATION("WebCore.IndexedDB.TombstoneCrawler.StartError",
                                leveldb_env::GetLevelDBStatusUMAValue(status),
                                leveldb_env::LEVELDB_STATUS_MAX);
    return status;
  }
  for (const base::string16& name : names) {
    database_metadata_.push_back(IndexedDBDatabaseMetadata());
    bool success = false;
    status = backing_store_->GetIDBDatabaseMetaData(
        name, &database_metadata_.back(), &success);
    database_metadata_.back().name = name;
    if (!success) {
      LOCAL_HISTOGRAM_ENUMERATION(
          "WebCore.IndexedDB.TombstoneCrawler.StartError",
          leveldb_env::GetLevelDBStatusUMAValue(status),
          leveldb_env::LEVELDB_STATUS_MAX);
      return status;
    }
    status = backing_store_->GetObjectStores(
        database_metadata_.back().id, &database_metadata_.back().object_stores);
    if (!status.ok()) {
      LOCAL_HISTOGRAM_ENUMERATION(
          "WebCore.IndexedDB.TombstoneCrawler.StartError",
          leveldb_env::GetLevelDBStatusUMAValue(status),
          leveldb_env::LEVELDB_STATUS_MAX);
      return status;
    }
  }

  return status;
}

void IndexedDBTombstoneCrawler::CrawlLoop() {
  static const char kUmaPrefix[] = "WebCore.IndexedDB.TombstoneCrawler.";
  leveldb::Status s;
  CrawlStatus status = DoCrawl(&s);

  if (mode_ == Mode::DELETION)
    FlushDeletions();

  if (status == CrawlStatus::PENDING) {
    task_runner_->PostTask(FROM_HERE,
                           base::Bind(&IndexedDBTombstoneCrawler::CrawlLoop,
                                      ptr_factory_.GetWeakPtr()));
    return;
  }
  DCHECK_EQ(status, CrawlStatus::STOPPED);

  std::string uma_count_label = kUmaPrefix;
  std::string uma_size_label = kUmaPrefix;

  switch (state_) {
    case State::DONE_NEW_CONNECTION:
      uma_count_label.append("ConnectionOpened.");
      uma_size_label.append("ConnectionOpened.");
      break;
    case State::DONE_TIMOUT:
      uma_count_label.append("TimeoutReached.");
      uma_size_label.append("TimeoutReached.");
      break;
    case State::DONE_REACHED_MAX:
      uma_count_label.append("MaxIterations.");
      uma_size_label.append("MaxIterations.");
      break;
    case State::DONE_ERROR:
      LOCAL_HISTOGRAM_ENUMERATION(
          "WebCore.IndexedDB.TombstoneCrawler.CrawlError",
          leveldb_env::GetLevelDBStatusUMAValue(s),
          leveldb_env::LEVELDB_STATUS_MAX);
      uma_count_label.append("CrawlError.");
      uma_size_label.append("CrawlError.");
      break;
    case State::DONE_COMPLETE:
      uma_count_label.append("Complete.");
      uma_size_label.append("Complete.");
      break;
    default:
      NOTREACHED();
  }

  switch (mode_) {
    case Mode::STATISTICS:
      uma_count_label.append("NumTombstones");
      uma_size_label.append("TombstonesSize");
      break;
    case Mode::DELETION:
      uma_count_label.append("NumDeletedTombstones");
      uma_size_label.append("DeletedTombstonesSize");
    default:
      NOTREACHED();
  }

  base::HistogramBase* count_histogram = base::Histogram::FactoryGet(
      uma_count_label, 1, 1000000, 50, base::HistogramBase::kNoFlags);
  base::HistogramBase* size_histogram = base::Histogram::FactoryGet(
      uma_size_label, 1, 1000000, 50, base::HistogramBase::kNoFlags);

  if (count_histogram)
    count_histogram->Add(metrics_.total_tombstones);
  if (size_histogram)
    size_histogram->Add(metrics_.total_tombstones_size);

  LOG(ERROR) << "Got tombstones: " << metrics_.total_tombstones;
  LOG(ERROR) << "Iterations " << num_iterations_;

  crawl_deadline_timer_.Stop();
  ClearAndCallDone();
}

bool IndexedDBTombstoneCrawler::ShouldContinueIteration(
    IndexedDBTombstoneCrawler::CrawlStatus* crawl_status,
    leveldb::Status* leveldb_status,
    size_t* round_iters) {
  ++num_iterations_;
  ++(*round_iters);

  if (IsStopped()) {
    *crawl_status = CrawlStatus::STOPPED;
    return false;
  }

  if (!iterator_->Valid()) {
    *leveldb_status = iterator_->status();
    if (!leveldb_status->ok()) {
      state_ = State::DONE_ERROR;
      *crawl_status = CrawlStatus::STOPPED;
      return false;
    }
    state_ = State::DONE_COMPLETE;
    *crawl_status = CrawlStatus::STOPPED;
    return false;
  }
  if (*round_iters >= max_round_iterations_) {
    LOG(ERROR) << "hit iter limit for round";
    *crawl_status = CrawlStatus::PENDING;
    return false;
  }
  if (num_iterations_ >= max_iterations_) {
    state_ = State::DONE_REACHED_MAX;
    *crawl_status = CrawlStatus::STOPPED;
    return false;
  }
  return true;
}

IndexedDBTombstoneCrawler::CrawlStatus IndexedDBTombstoneCrawler::DoCrawl(
    leveldb::Status* leveldb_status) {
  if (IsStopped())
    return CrawlStatus::STOPPED;

  size_t round_iters = 0;
  CrawlStatus crawl_status;

  for (; crawl_state_.metadata_idx < database_metadata_.size();
       ++crawl_state_.metadata_idx) {
    const IndexedDBDatabaseMetadata& database =
        database_metadata_[crawl_state_.metadata_idx];
    LOG(ERROR) << "processing database " << database.name;
    if (database.object_stores.empty())
      continue;

    if (!crawl_state_.object_store_it)
      crawl_state_.object_store_it = database.object_stores.begin();

    for (; crawl_state_.object_store_it.value() != database.object_stores.end();
         crawl_state_.object_store_it.value()++) {
      const IndexedDBObjectStoreMetadata& object_store =
          crawl_state_.object_store_it.value()->second;

      if (object_store.indexes.empty())
        continue;

      if (!crawl_state_.index_it)
        crawl_state_.index_it = object_store.indexes.begin();

      for (; crawl_state_.index_it.value() != object_store.indexes.end();
           crawl_state_.index_it.value()++) {
        const IndexedDBIndexMetadata& index =
            crawl_state_.index_it.value()->second;

        if (crawl_state_.curr_index_key) {
          iterator_->Seek(crawl_state_.curr_index_key.value().Encode());
          if (!ShouldContinueIteration(&crawl_status, leveldb_status,
                                       &round_iters)) {
            return crawl_status;
          }
          iterator_->Next();
          if (!ShouldContinueIteration(&crawl_status, leveldb_status,
                                       &round_iters)) {
            return crawl_status;
          }
        } else {
          iterator_->Seek(IndexDataKey::EncodeMinKey(
              database.id, object_store.id, index.id));
          if (!ShouldContinueIteration(&crawl_status, leveldb_status,
                                       &round_iters)) {
            return crawl_status;
          }
        }

        while (iterator_->Valid()) {
          base::StringPiece index_key_str = MakeStringPiece(iterator_->key());
          base::StringPiece index_value_str =
              MakeStringPiece(iterator_->value());
          // See if we've reached the end of the current index or all indexes.
          crawl_state_.curr_index_key.emplace(IndexDataKey());
          if (!IndexDataKey::Decode(&index_key_str,
                                    &crawl_state_.curr_index_key.value()) ||
              crawl_state_.curr_index_key.value().IndexId() != index.id) {
            break;
          }

          size_t entry_size =
              iterator_->key().size() + iterator_->value().size();

          int64_t index_data_version;
          std::unique_ptr<IndexedDBKey> primary_key;

          if (!DecodeVarInt(&index_value_str, &index_data_version)) {
            ++metrics_.invalid_index_values;
            iterator_->Next();
            if (!ShouldContinueIteration(&crawl_status, leveldb_status,
                                         &round_iters)) {
              return crawl_status;
            }
            continue;
          }
          std::string encoded_primary_key = index_value_str.as_string();
          std::string exists_key = ExistsEntryKey::Encode(
              database.id, object_store.id, encoded_primary_key);

          std::string exists_value;
          leveldb::Status s =
              database_->Get(leveldb::ReadOptions(), exists_key, &exists_value);
          if (!s.ok()) {
            ++metrics_.errors_reading_exists_table;
            iterator_->Next();
            if (!ShouldContinueIteration(&crawl_status, leveldb_status,
                                         &round_iters)) {
              return crawl_status;
            }
            continue;
          }
          base::StringPiece exists_value_piece(exists_value);
          int64_t decoded_exists_version;
          if (!DecodeInt(&exists_value_piece, &decoded_exists_version) ||
              !exists_value_piece.empty()) {
            ++metrics_.invalid_exists_values;
            iterator_->Next();
            if (!ShouldContinueIteration(&crawl_status, leveldb_status,
                                         &round_iters)) {
              return crawl_status;
            }
            continue;
          }

          if (decoded_exists_version != index_data_version) {
            if (mode_ == Mode::DELETION) {
              curr_deletion_batch_.Delete(iterator_->key());
            }
            ++metrics_.total_tombstones;
            metrics_.total_tombstones_size += entry_size;
          }

          iterator_->Next();
          if (!ShouldContinueIteration(&crawl_status, leveldb_status,
                                       &round_iters)) {
            return crawl_status;
          }
        }
        crawl_state_.curr_index_key = base::nullopt;
      }
      crawl_state_.index_it = base::nullopt;
    }
    crawl_state_.object_store_it = base::nullopt;
  }
  state_ = State::DONE_COMPLETE;
  return CrawlStatus::STOPPED;
}

}  // namespace content
