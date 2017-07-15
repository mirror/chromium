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
    IndexedDBTombstoneCrawler::Mode mode,
    base::TimeDelta max_crawl_time,
    std::unique_ptr<base::Clock> clock,
    base::OnceClosure on_complete)
    : mode_(mode),
      task_runner_(base::SequencedTaskRunnerHandle::Get()),
      end_time_(clock->Now() + max_crawl_time),
      clock_(std::move(clock)),
      on_complete_(std::move(on_complete)),
      ptr_factory_(this) {}

IndexedDBTombstoneCrawler::~IndexedDBTombstoneCrawler() {
  if (snapshot_) {
    database_->ReleaseSnapshot(snapshot_);
  }
}

IndexedDBTombstoneCrawler::CrawlState::CrawlState() = default;
IndexedDBTombstoneCrawler::CrawlState::~CrawlState() = default;

base::WeakPtr<IndexedDBTombstoneCrawler>
IndexedDBTombstoneCrawler::StartAndDestroy(
    leveldb::DB* database,
    scoped_refptr<IndexedDBBackingStore> backing_store) {
  CHECK(!started_);
  backing_store_ = std::move(backing_store);
  database_ = database;
  snapshot_ = database_->GetSnapshot();
  leveldb::ReadOptions iterator_options;
  iterator_options.fill_cache = false;
  iterator_options.verify_checksums = true;
  iterator_options.snapshot = snapshot_;
  iterator_.reset(database->NewIterator(iterator_options));
  started_ = true;

  leveldb::Status status = leveldb::Status::OK();
  std::vector<base::string16> names = backing_store_->GetDatabaseNames(&status);
  if (!status.ok()) {
    LOCAL_HISTOGRAM_ENUMERATION("WebCore.IndexedDB.TombstoneCrawler.StartError",
                                leveldb_env::GetLevelDBStatusUMAValue(status),
                                leveldb_env::LEVELDB_STATUS_MAX);
    delete this;
    return base::WeakPtr<IndexedDBTombstoneCrawler>();
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
      delete this;
      return base::WeakPtr<IndexedDBTombstoneCrawler>();
    }
    status = backing_store_->GetObjectStores(
        database_metadata_.back().id, &database_metadata_.back().object_stores);
    if (!status.ok()) {
      LOCAL_HISTOGRAM_ENUMERATION(
          "WebCore.IndexedDB.TombstoneCrawler.StartError",
          leveldb_env::GetLevelDBStatusUMAValue(status),
          leveldb_env::LEVELDB_STATUS_MAX);
      delete this;
      return base::WeakPtr<IndexedDBTombstoneCrawler>();
    }
  }

  task_runner_->PostTask(
      FROM_HERE, base::Bind(&IndexedDBTombstoneCrawler::CrawlOrExitAndDestruct,
                            base::Unretained(this)));
  return ptr_factory_.GetWeakPtr();
}

void IndexedDBTombstoneCrawler::CrawlOrExitAndDestruct() {
  leveldb::Status s;
  CrawlStatus status = DoCrawl(&s);
  switch (status) {
    case CrawlStatus::MORE_ENTRIES:
      task_runner_->PostDelayedTask(
          FROM_HERE,
          base::Bind(&IndexedDBTombstoneCrawler::CrawlOrExitAndDestruct,
                     base::Unretained(this)),
          base::TimeDelta::FromMilliseconds(10));
      return;
    case CrawlStatus::LEVELDB_ERROR:
      LOCAL_HISTOGRAM_ENUMERATION(
          "WebCore.IndexedDB.TombstoneCrawler.CrawlError",
          leveldb_env::GetLevelDBStatusUMAValue(s),
          leveldb_env::LEVELDB_STATUS_MAX);
      break;
    case CrawlStatus::REACHED_MAX:
    case CrawlStatus::REACHED_TIME:
      LOCAL_HISTOGRAM_ENUMERATION(
          "WebCore.IndexedDB.TombstoneCrawler.EarlyExit",
          status == CrawlStatus::REACHED_MAX ? 0 : 1, 2);
    case CrawlStatus::DONE:
      switch (mode_) {
        case Mode::STATISTICS:
          LOCAL_HISTOGRAM_COUNTS_1000000(
              "WebCore.IndexedDB.TombstoneCrawler.NumTombstones",
              metrics_.total_tombstones);
          LOCAL_HISTOGRAM_COUNTS_1000000(
              "WebCore.IndexedDB.TombstoneCrawler.TombstonesSize",
              metrics_.total_tombstones_size);
          break;
        case Mode::DELETION:
          LOCAL_HISTOGRAM_COUNTS_1000000(
              "WebCore.IndexedDB.TombstoneCrawler.NumTombstonesDeleted",
              metrics_.total_tombstones);
          LOCAL_HISTOGRAM_COUNTS_1000000(
              "WebCore.IndexedDB.TombstoneCrawler.TombstonesDeletionSize",
              metrics_.total_tombstones_size);
      }
      break;
    default:
      NOTREACHED();
  }

  if (mode_ == Mode::DELETION) {
    FlushDeletions();
  }
  base::OnceClosure done_closure = std::move(on_complete_);
  delete this;
  std::move(done_closure).Run();
  return;
}

bool IndexedDBTombstoneCrawler::ShouldStopAfterIteration(
    IndexedDBTombstoneCrawler::CrawlStatus* crawl_status,
    leveldb::Status* leveldb_status,
    size_t* round_iters) {
  ++iterations_;
  ++(*round_iters);

  if (!iterator_->Valid()) {
    *leveldb_status = iterator_->status();
    if (!leveldb_status->ok()) {
      *crawl_status = CrawlStatus::LEVELDB_ERROR;
      return true;
    }
    *crawl_status = CrawlStatus::DONE;
    LOG(ERROR) << "reached end of everything";
    LOG(ERROR) << "Got tombstones: " << metrics_.total_tombstones;
    LOG(ERROR) << "Iterations " << iterations_;
    return true;
  }
  if (*round_iters >= max_iterations_per_crawl_) {
    LOG(ERROR) << "hit iter limit for round";
    *crawl_status = CrawlStatus::MORE_ENTRIES;
    return true;
  }
  if (iterations_ >= max_iterations_) {
    LOG(ERROR) << "hit max iters";
    LOG(ERROR) << "Got tombstones: " << metrics_.total_tombstones;
    LOG(ERROR) << "Iterations " << iterations_;
    *crawl_status = CrawlStatus::REACHED_MAX;
    return true;
  }
  if (clock_->Now() >= end_time_) {
    LOG(ERROR) << "over time";
    LOG(ERROR) << "Got tombstones: " << metrics_.total_tombstones;
    LOG(ERROR) << "Iterations " << iterations_;
    *crawl_status = CrawlStatus::REACHED_TIME;
    return true;
  }
  return false;
}

void IndexedDBTombstoneCrawler::FlushDeletions() {
  LOG(ERROR) << "flushing deletions " << curr_deletion_batch_.ApproximateSize();
  leveldb::WriteOptions options;
  options.sync = true;
  database_->Write(options, &curr_deletion_batch_);
  curr_deletion_batch_.Clear();
}

IndexedDBTombstoneCrawler::CrawlStatus IndexedDBTombstoneCrawler::DoCrawl(
    leveldb::Status* leveldb_status) {
  leveldb::ReadOptions exists_options;
  exists_options.snapshot = snapshot_;

  size_t round_iters = 0;
  CrawlStatus crawl_status;

  for (; crawl_state_.database < database_metadata_.size();
       ++crawl_state_.database) {
    const IndexedDBDatabaseMetadata& database =
        database_metadata_[crawl_state_.database];
    LOG(ERROR) << "processing database " << database.name;
    if (database.object_stores.empty()) {
      LOG(ERROR) << "no object stores";
      continue;
    }
    if (!crawl_state_.object_store_it) {
      LOG(ERROR) << "first object store";
      crawl_state_.object_store_it = database.object_stores.begin();
    }

    for (; crawl_state_.object_store_it.value() != database.object_stores.end();
         crawl_state_.object_store_it.value()++) {
      const IndexedDBObjectStoreMetadata& object_store =
          crawl_state_.object_store_it.value()->second;
      LOG(ERROR) << "processing os " << object_store.name;

      if (object_store.indexes.empty()) {
        LOG(ERROR) << "no indexes";
        continue;
      }
      if (!crawl_state_.index_it) {
        crawl_state_.index_it = object_store.indexes.begin();
      }
      for (; crawl_state_.index_it.value() != object_store.indexes.end();
           crawl_state_.index_it.value()++) {
        const IndexedDBIndexMetadata& index =
            crawl_state_.index_it.value()->second;

        if (crawl_state_.curr_index_key) {
          iterator_->Seek(crawl_state_.curr_index_key.value().Encode());
          if (ShouldStopAfterIteration(&crawl_status, leveldb_status,
                                       &round_iters)) {
            return crawl_status;
          }
          iterator_->Next();
          if (ShouldStopAfterIteration(&crawl_status, leveldb_status,
                                       &round_iters)) {
            return crawl_status;
          }
        } else {
          iterator_->Seek(IndexDataKey::EncodeMinKey(
              database.id, object_store.id, index.id));
          if (ShouldStopAfterIteration(&crawl_status, leveldb_status,
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
            continue;
          }
          std::string encoded_primary_key = index_value_str.as_string();
          std::string exists_key = ExistsEntryKey::Encode(
              database.id, object_store.id, encoded_primary_key);

          std::string exists_value;
          leveldb::Status s =
              database_->Get(exists_options, exists_key, &exists_value);
          if (!s.ok()) {
            ++metrics_.errors_reading_exists_table;
            continue;
          }
          base::StringPiece exists_value_piece(exists_value);
          int64_t decoded_exists_version;
          if (!DecodeInt(&exists_value_piece, &decoded_exists_version) ||
              !exists_value_piece.empty()) {
            ++metrics_.invalid_exists_values;
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
          if (ShouldStopAfterIteration(&crawl_status, leveldb_status,
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

  if (mode_ == Mode::DELETION) {
    FlushDeletions();
  }
  LOG(ERROR) << "Got tombstones: " << metrics_.total_tombstones;
  LOG(ERROR) << "Iterations " << iterations_;
  return CrawlStatus::DONE;
}

}  // namespace content
