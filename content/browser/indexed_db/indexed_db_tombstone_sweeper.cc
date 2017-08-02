// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/indexed_db/indexed_db_tombstone_sweeper.h"

#include "base/metrics/histogram_macros.h"
#include "base/rand_util.h"
#include "base/sequenced_task_runner.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "content/browser/indexed_db/indexed_db_backing_store.h"
#include "third_party/leveldatabase/env_chromium.h"
#include "third_party/leveldatabase/src/include/leveldb/db.h"
#include "third_party/leveldatabase/src/include/leveldb/iterator.h"

namespace content {
namespace {

using StopReason = IndexedDBPreCloseTaskQueue::StopReason;

// TODO(dmurph): Make common implementation in env_chromium.h
base::StringPiece MakeStringPiece(const leveldb::Slice& s) {
  return base::StringPiece(s.data(), s.size());
}

}  // namespace

template <typename T>
WrappingIterator<T>::WrappingIterator() {}

template <typename T>
WrappingIterator<T>::WrappingIterator(const T* container,
                                      size_t start_position) {
  container_ = container;
  valid_ = true;
  iterations_done_ = 0;
  DCHECK_LT(start_position, container_->size());
  inner_ = container_->begin();
  std::advance(inner_, start_position);
  DCHECK(inner_ != container_->end());
}

template <typename T>
WrappingIterator<T>::~WrappingIterator() {}

template <typename T>
void WrappingIterator<T>::Next() {
  DCHECK(valid_);
  iterations_done_++;
  if (iterations_done_ >= container_->size()) {
    valid_ = false;
    return;
  }
  inner_++;
  if (inner_ == container_->end()) {
    inner_ = container_->begin();
  }
}

template <typename T>
const typename T::value_type& WrappingIterator<T>::Value() const {
  CHECK(valid_);
  return *inner_;
}

IndexedDBTombstoneSweeper::IndexedDBTombstoneSweeper(Mode mode,
                                                     int round_iterations,
                                                     int max_iterations,
                                                     leveldb::DB* database)
    : mode_(mode),
      max_round_iterations_(round_iterations),
      max_iterations_(max_iterations),
      database_(database),
      ptr_factory_(this) {
  crawl_state_.start_database_seed = base::RandUint64();
  crawl_state_.start_object_store_seed = base::RandUint64();
  crawl_state_.start_index_seed = base::RandUint64();
}

IndexedDBTombstoneSweeper::~IndexedDBTombstoneSweeper() {}

void IndexedDBTombstoneSweeper::SetMetadata(
    std::vector<IndexedDBDatabaseMetadata> const* metadata) {
  database_metadata_ = metadata;
}

IndexedDBTombstoneSweeper::CrawlState::CrawlState() = default;

IndexedDBTombstoneSweeper::CrawlState::~CrawlState() = default;

void IndexedDBTombstoneSweeper::Stop(StopReason reason) {
  leveldb::Status s;
  RecordUMAStats(reason, base::nullopt, s);
}

bool IndexedDBTombstoneSweeper::RunRound() {
  DCHECK(database_metadata_);
  leveldb::Status s;
  Status status = DoCrawl(&s);

  if (status != Status::DONE_ERROR && mode_ == Mode::DELETION) {
    s = FlushDeletions();
    if (!s.ok())
      status = Status::DONE_ERROR;
  }

  if (status == Status::SWEEPING)
    return false;

  RecordUMAStats(base::nullopt, status, s);
  return true;
}

void IndexedDBTombstoneSweeper::RecordUMAStats(
    base::Optional<StopReason> stop_reason,
    base::Optional<IndexedDBTombstoneSweeper::Status> status,
    const leveldb::Status& leveldb_error) {
  static const char kUmaPrefix[] = "WebCore.IndexedDB.TombstoneSweeper.";
  DCHECK(stop_reason || status);
  DCHECK(!stop_reason || !status);

  std::string uma_count_label = kUmaPrefix;
  std::string uma_size_label = kUmaPrefix;

  if (stop_reason) {
    switch (stop_reason.value()) {
      case StopReason::NEW_CONNECTION:
        uma_count_label.append("ConnectionOpened.");
        uma_size_label.append("ConnectionOpened.");
        break;
      case StopReason::TIMEOUT:
        uma_count_label.append("TimeoutReached.");
        uma_size_label.append("TimeoutReached.");
        break;
      default:
        NOTREACHED();
    }
  } else if (status) {
    switch (status.value()) {
      case Status::DONE_REACHED_MAX:
        uma_count_label.append("MaxIterations.");
        uma_size_label.append("MaxIterations.");
        break;
      case Status::DONE_ERROR:
        LOCAL_HISTOGRAM_ENUMERATION(
            "WebCore.IndexedDB.TombstoneSweeper.SweepError",
            leveldb_env::GetLevelDBStatusUMAValue(leveldb_error),
            leveldb_env::LEVELDB_STATUS_MAX);
        uma_count_label.append("SweepError.");
        uma_size_label.append("SweepError.");
        break;
      case Status::DONE_COMPLETE:
        uma_count_label.append("Complete.");
        uma_size_label.append("Complete.");
        break;
      default:
        NOTREACHED();
    }
  } else {
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

  if (status && status.value() == Status::DONE_COMPLETE &&
      mode_ == Mode::DELETION) {
    LOCAL_HISTOGRAM_TIMES(
        "WebCore.IndexedDB.TombstoneSweeper.Complete.DeletionTotalTime",
        total_deletion_time_);
  }

  base::HistogramBase* count_histogram = base::Histogram::FactoryGet(
      uma_count_label, 1, 1000000, 50, base::HistogramBase::kNoFlags);
  base::HistogramBase* size_histogram = base::Histogram::FactoryGet(
      uma_size_label, 1, 1000000, 50, base::HistogramBase::kNoFlags);

  if (count_histogram)
    count_histogram->Add(metrics_.total_tombstones);
  if (size_histogram)
    size_histogram->Add(metrics_.total_tombstones_size);

  // TODO(next patch): remove all logging, just here to verify working before
  // tests.
  LOG(ERROR) << "Got tombstones: " << metrics_.total_tombstones;
  LOG(ERROR) << "Iterations " << num_iterations_;
}

leveldb::Status IndexedDBTombstoneSweeper::FlushDeletions() {
  if (!has_writes_)
    return leveldb::Status::OK();
  base::TimeTicks start = base::TimeTicks::Now();

  leveldb::Status status =
      database_->Write(leveldb::WriteOptions(), &curr_deletion_batch_);
  curr_deletion_batch_.Clear();
  has_writes_ = false;

  if (!status.ok()) {
    LOCAL_HISTOGRAM_ENUMERATION(
        "WebCore.IndexedDB.TombstoneSweeper.DeletionWriteError",
        leveldb_env::GetLevelDBStatusUMAValue(status),
        leveldb_env::LEVELDB_STATUS_MAX);
    return status;
  }

  base::TimeDelta diff = base::TimeTicks::Now() - start;
  total_deletion_time_ += diff;
  LOCAL_HISTOGRAM_TIMES("WebCore.IndexedDB.TombstoneSweeper.DeletionCommitTime",
                        diff);
  return status;
}

bool IndexedDBTombstoneSweeper::ShouldContinueIteration(
    IndexedDBTombstoneSweeper::Status* crawl_status,
    leveldb::Status* leveldb_status,
    int* round_iterations) {
  ++num_iterations_;
  ++(*round_iterations);

  if (!iterator_->Valid()) {
    *leveldb_status = iterator_->status();
    if (!leveldb_status->ok()) {
      *crawl_status = Status::DONE_ERROR;
      return false;
    }
    *crawl_status = Status::SWEEPING;
    return true;
  }
  if (*round_iterations >= max_round_iterations_) {
    LOG(ERROR) << "round done";
    *crawl_status = Status::SWEEPING;
    return false;
  }
  if (num_iterations_ >= max_iterations_) {
    *crawl_status = Status::DONE_REACHED_MAX;
    return false;
  }
  return true;
}

IndexedDBTombstoneSweeper::Status IndexedDBTombstoneSweeper::DoCrawl(
    leveldb::Status* leveldb_status) {
  int round_iterations = 0;
  Status crawl_status;
  if (database_metadata_->empty())
    return Status::DONE_COMPLETE;

  if (!iterator_) {
    leveldb::ReadOptions iterator_options;
    iterator_options.fill_cache = false;
    iterator_options.verify_checksums = true;
    iterator_.reset(database_->NewIterator(iterator_options));
  }

  if (!crawl_state_.database_it) {
    size_t start_database_idx = static_cast<size_t>(
        crawl_state_.start_database_seed % database_metadata_->size());
    crawl_state_.database_it = WrappingIterator<DatabaseMetadataVector>(
        database_metadata_, start_database_idx);
  }
  // Loop conditions facilitate starting at random index.
  for (; crawl_state_.database_it.value().IsValid();
       crawl_state_.database_it.value().Next()) {
    const IndexedDBDatabaseMetadata& database =
        crawl_state_.database_it.value().Value();
    if (database.object_stores.empty())
      continue;

    if (!crawl_state_.object_store_it) {
      size_t start_object_store_idx = static_cast<size_t>(
          crawl_state_.start_object_store_seed % database.object_stores.size());
      crawl_state_.object_store_it = WrappingIterator<ObjectStoreMetadataMap>(
          &database.object_stores, start_object_store_idx);
    }
    // Loop conditions facilitate starting at random index.
    for (; crawl_state_.object_store_it.value().IsValid();
         crawl_state_.object_store_it.value().Next()) {
      const IndexedDBObjectStoreMetadata& object_store =
          crawl_state_.object_store_it.value().Value().second;

      if (object_store.indexes.empty())
        continue;

      if (!crawl_state_.index_it) {
        size_t start_index_idx = static_cast<size_t>(
            crawl_state_.start_index_seed % object_store.indexes.size());
        crawl_state_.index_it = WrappingIterator<IndexMetadataMap>(
            &object_store.indexes, start_index_idx);
      }
      // Loop conditions facilitate starting at random index.
      for (; crawl_state_.index_it.value().IsValid();
           crawl_state_.index_it.value().Next()) {
        const IndexedDBIndexMetadata& index =
            crawl_state_.index_it.value().Value().second;

        bool can_continue =
            IterateIndex(database.id, object_store.id, index, &crawl_status,
                         leveldb_status, &round_iterations);
        if (!can_continue)
          return crawl_status;
      }
      crawl_state_.index_it = base::nullopt;
    }
    crawl_state_.object_store_it = base::nullopt;
  }
  return Status::DONE_COMPLETE;
}

bool IndexedDBTombstoneSweeper::IterateIndex(
    int64_t database_id,
    int64_t object_store_id,
    const IndexedDBIndexMetadata& index,
    IndexedDBTombstoneSweeper::Status* crawl_status,
    leveldb::Status* leveldb_status,
    int* round_iterations) {
  // If the sweeper exited early from an index scan, continue where it left off.
  if (crawl_state_.curr_index_key) {
    iterator_->Seek(crawl_state_.curr_index_key.value().Encode());
    if (!ShouldContinueIteration(crawl_status, leveldb_status,
                                 round_iterations)) {
      return false;
    }
    // Start at the first unvisited value.
    iterator_->Next();
    if (!ShouldContinueIteration(crawl_status, leveldb_status,
                                 round_iterations)) {
      return false;
    }
  } else {
    iterator_->Seek(
        IndexDataKey::EncodeMinKey(database_id, object_store_id, index.id));
    if (!ShouldContinueIteration(crawl_status, leveldb_status,
                                 round_iterations)) {
      return false;
    }
  }

  // TODO(next patch): split out index iteration
  while (iterator_->Valid()) {
    base::StringPiece index_key_str = MakeStringPiece(iterator_->key());
    base::StringPiece index_value_str = MakeStringPiece(iterator_->value());
    // See if we've reached the end of the current index or all indexes.
    crawl_state_.curr_index_key.emplace(IndexDataKey());
    if (!IndexDataKey::Decode(&index_key_str,
                              &crawl_state_.curr_index_key.value()) ||
        crawl_state_.curr_index_key.value().IndexId() != index.id) {
      break;
    }

    size_t entry_size = iterator_->key().size() + iterator_->value().size();

    int64_t index_data_version;
    std::unique_ptr<IndexedDBKey> primary_key;

    if (!DecodeVarInt(&index_value_str, &index_data_version)) {
      ++metrics_.num_invalid_index_values;
      iterator_->Next();
      if (!ShouldContinueIteration(crawl_status, leveldb_status,
                                   round_iterations)) {
        return false;
      }
      continue;
    }
    std::string encoded_primary_key = index_value_str.as_string();
    std::string exists_key = ExistsEntryKey::Encode(
        database_id, object_store_id, encoded_primary_key);

    std::string exists_value;
    leveldb::Status s =
        database_->Get(leveldb::ReadOptions(), exists_key, &exists_value);
    if (!s.ok()) {
      ++metrics_.num_errors_reading_exists_table;
      iterator_->Next();
      if (!ShouldContinueIteration(crawl_status, leveldb_status,
                                   round_iterations)) {
        return false;
      }
      continue;
    }
    base::StringPiece exists_value_piece(exists_value);
    int64_t decoded_exists_version;
    if (!DecodeInt(&exists_value_piece, &decoded_exists_version) ||
        !exists_value_piece.empty()) {
      ++metrics_.num_invalid_exists_values;
      iterator_->Next();
      if (!ShouldContinueIteration(crawl_status, leveldb_status,
                                   round_iterations)) {
        return false;
      }
      continue;
    }

    if (decoded_exists_version != index_data_version) {
      if (mode_ == Mode::DELETION) {
        has_writes_ = true;
        curr_deletion_batch_.Delete(iterator_->key());
      }
      ++metrics_.total_tombstones;
      metrics_.total_tombstones_size += entry_size;
    }

    iterator_->Next();
    if (!ShouldContinueIteration(crawl_status, leveldb_status,
                                 round_iterations)) {
      return false;
    }
  }
  crawl_state_.curr_index_key = base::nullopt;
  return true;
}

}  // namespace content
