// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/leveldb_wrapper_impl.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/memory_dump_manager.h"
#include "base/trace_event/process_memory_dump.h"
#include "components/leveldb/public/cpp/util.h"
#include "content/public/browser/browser_thread.h"

namespace content {
namespace {
using leveldb::mojom::BatchedOperation;
using leveldb::mojom::BatchedOperationPtr;
}  // namespace

void LevelDBWrapperImpl::Delegate::MigrateData(
    base::OnceCallback<void(std::unique_ptr<ValueMap>)> callback) {
  std::move(callback).Run(nullptr);
}

std::vector<LevelDBWrapperImpl::Change> LevelDBWrapperImpl::Delegate::FixUpData(
    const ValueMap& data) {
  return std::vector<Change>();
}

void LevelDBWrapperImpl::Delegate::OnMapLoaded(leveldb::mojom::DatabaseError) {}

bool LevelDBWrapperImpl::s_aggressive_flushing_enabled_ = false;

LevelDBWrapperImpl::RateLimiter::RateLimiter(size_t desired_rate,
                                            base::TimeDelta time_quantum)
    : rate_(desired_rate), samples_(0), time_quantum_(time_quantum) {
  DCHECK_GT(desired_rate, 0ul);
}

base::TimeDelta LevelDBWrapperImpl::RateLimiter::ComputeTimeNeeded() const {
  return time_quantum_ * (samples_ / rate_);
}

base::TimeDelta LevelDBWrapperImpl::RateLimiter::ComputeDelayNeeded(
    const base::TimeDelta elapsed_time) const {
  base::TimeDelta time_needed = ComputeTimeNeeded();
  if (time_needed > elapsed_time)
    return time_needed - elapsed_time;
  return base::TimeDelta();
}

LevelDBWrapperImpl::CommitBatch::CommitBatch() : clear_all_first(false) {}
LevelDBWrapperImpl::CommitBatch::~CommitBatch() {}

LevelDBWrapperImpl::LevelDBWrapperImpl(
    leveldb::mojom::LevelDBDatabase* database,
    const std::string& prefix,
    size_t max_size,
    base::TimeDelta default_commit_delay,
    int max_bytes_per_hour,
    int max_commits_per_hour,
    Delegate* delegate)
    : prefix_(leveldb::StdStringToUint8Vector(prefix)),
      delegate_(delegate),
      database_(database),
      bytes_used_(0),
      max_size_(max_size),
      start_time_(base::TimeTicks::Now()),
      default_commit_delay_(default_commit_delay),
      data_rate_limiter_(max_bytes_per_hour, base::TimeDelta::FromHours(1)),
      commit_rate_limiter_(max_commits_per_hour, base::TimeDelta::FromHours(1)),
      weak_ptr_factory_(this) {
  bindings_.set_connection_error_handler(base::Bind(
      &LevelDBWrapperImpl::OnConnectionError, base::Unretained(this)));
}

LevelDBWrapperImpl::~LevelDBWrapperImpl() {
  DCHECK(!has_pending_load_tasks());
  if (commit_batch_)
    CommitChanges();
}

std::unique_ptr<LevelDBWrapperImpl> LevelDBWrapperImpl::ForkToNewPrefix(
    const std::string& new_prefix,
    Delegate* delegate) {
  std::unique_ptr<LevelDBWrapperImpl> forked_wrapper =
      base::MakeUnique<LevelDBWrapperImpl>(
          database_, new_prefix, max_size_, default_commit_delay_,
          data_rate_limiter_.rate(), commit_rate_limiter_.rate(), delegate);

  forked_wrapper->map_state_ = MapLoadingState::LOADING_FROM_FORK;

  if (map_state_ == MapLoadingState::LOADED) {
    DoForkOperation(forked_wrapper->weak_ptr_factory_.GetWeakPtr());
  } else {
    LoadMap(base::BindOnce(&LevelDBWrapperImpl::DoForkOperation,
                           weak_ptr_factory_.GetWeakPtr(),
                           forked_wrapper->weak_ptr_factory_.GetWeakPtr()));
  }
  return forked_wrapper;
}

void LevelDBWrapperImpl::Bind(mojom::LevelDBWrapperRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void LevelDBWrapperImpl::EnableAggressiveCommitDelay() {
  s_aggressive_flushing_enabled_ = true;
}

void LevelDBWrapperImpl::ScheduleImmediateCommit() {
  if (!on_load_complete_tasks_.empty()) {
    LoadMap(base::BindOnce(&LevelDBWrapperImpl::ScheduleImmediateCommit,
                           base::Unretained(this)));
    return;
  }

  if (!database_ || !commit_batch_)
    return;
  CommitChanges();
}

void LevelDBWrapperImpl::OnMemoryDump(
    const std::string& name,
    base::trace_event::ProcessMemoryDump* pmd) {
  if (map_state_ != MapLoadingState::LOADED)
    return;

  const char* system_allocator_name =
      base::trace_event::MemoryDumpManager::GetInstance()
          ->system_allocator_pool_name();
  if (commit_batch_) {
    size_t data_size = 0;
    for (const auto& key : commit_batch_->changed_keys)
      data_size += key.size();
    auto* commit_batch_mad = pmd->CreateAllocatorDump(name + "/commit_batch");
    commit_batch_mad->AddScalar(
        base::trace_event::MemoryAllocatorDump::kNameSize,
        base::trace_event::MemoryAllocatorDump::kUnitsBytes, data_size);
    if (system_allocator_name)
      pmd->AddSuballocation(commit_batch_mad->guid(), system_allocator_name);
  }

  // Do not add storage map usage if less than 1KB.
  if (bytes_used_ < 1024)
    return;

  auto* map_mad = pmd->CreateAllocatorDump(name + "/storage_map");
  map_mad->AddScalar(base::trace_event::MemoryAllocatorDump::kNameSize,
                     base::trace_event::MemoryAllocatorDump::kUnitsBytes,
                     bytes_used_);
  if (system_allocator_name)
    pmd->AddSuballocation(map_mad->guid(), system_allocator_name);
}

void LevelDBWrapperImpl::PurgeMemory() {
  if (map_state_ != MapLoadingState::LOADED ||  // We're not using any memory.
      commit_batch_ ||  // We leave things alone with changes pending.
      !database_) {  // Don't purge anything if we're not backed by a database.
    return;
  }

  map_.clear();
  map_state_ = MapLoadingState::INVALID;
}

void LevelDBWrapperImpl::AddObserver(
    mojom::LevelDBObserverAssociatedPtrInfo observer) {
  mojom::LevelDBObserverAssociatedPtr observer_ptr;
  observer_ptr.Bind(std::move(observer));
  observers_.AddPtr(std::move(observer_ptr));
}

void LevelDBWrapperImpl::Put(
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& value,
    const base::Optional<std::vector<uint8_t>>& client_old_value,
    const std::string& source,
    PutCallback callback) {
  if (map_state_ != MapLoadingState::LOADED) {
    LoadMap(base::BindOnce(&LevelDBWrapperImpl::Put, base::Unretained(this),
                           key, value, client_old_value, source,
                           base::Passed(&callback)));
    return;
  }
  bool has_old_item = false;
  size_t old_item_size = 0;
  auto found = map_.find(key);
  if (found != map_.end()) {
    if (found->second == value) {
      std::move(callback).Run(true);  // Key already has this value.
      return;
    }
    old_item_size = key.size() + found->second.size();
    has_old_item = true;
  }
  size_t new_item_size = key.size() + value.size();
  size_t new_bytes_used = bytes_used_ - old_item_size + new_item_size;

  // Only check quota if the size is increasing, this allows
  // shrinking changes to pre-existing maps that are over budget.
  if (new_item_size > old_item_size && new_bytes_used > max_size_) {
    std::move(callback).Run(false);
    return;
  }

  if (database_) {
    CreateCommitBatchIfNeeded();
    commit_batch_->changed_keys.insert(key);
  }

  std::vector<uint8_t> old_value;
  if (has_old_item)
    old_value.swap(map_[key]);
  map_[key] = value;
  bytes_used_ = new_bytes_used;
  if (!has_old_item) {
    // We added a new key/value pair.
    observers_.ForAllPtrs(
        [&key, &value, &source](mojom::LevelDBObserver* observer) {
          observer->KeyAdded(key, value, source);
        });
  } else {
    // We changed the value for an existing key.
    observers_.ForAllPtrs(
        [&key, &value, &source, &old_value](mojom::LevelDBObserver* observer) {
          observer->KeyChanged(key, value, old_value, source);
        });
  }
  std::move(callback).Run(true);
}

void LevelDBWrapperImpl::Delete(
    const std::vector<uint8_t>& key,
    const base::Optional<std::vector<uint8_t>>& client_old_value,
    const std::string& source,
    DeleteCallback callback) {
  if (map_state_ != MapLoadingState::LOADED) {
    LoadMap(base::BindOnce(&LevelDBWrapperImpl::Delete, base::Unretained(this),
                           key, client_old_value, source,
                           base::Passed(&callback)));
    return;
  }

  auto found = map_.find(key);
  if (found == map_.end()) {
    std::move(callback).Run(true);
    return;
  }

  if (database_) {
    CreateCommitBatchIfNeeded();
    commit_batch_->changed_keys.insert(std::move(found->first));
  }

  std::vector<uint8_t> old_value(std::move(found->second));
  map_.erase(found);
  bytes_used_ -= key.size() + old_value.size();
  observers_.ForAllPtrs(
      [&key, &source, &old_value](mojom::LevelDBObserver* observer) {
        observer->KeyDeleted(key, old_value, source);
      });
  std::move(callback).Run(true);
}

void LevelDBWrapperImpl::DeleteAll(const std::string& source,
                                   DeleteAllCallback callback) {
  if (map_state_ != MapLoadingState::LOADED) {
    LoadMap(base::BindOnce(&LevelDBWrapperImpl::DeleteAll,
                           base::Unretained(this), source,
                           base::Passed(&callback)));
    return;
  }

  if (map_.empty()) {
    std::move(callback).Run(true);
    return;
  }

  if (database_) {
    CreateCommitBatchIfNeeded();
    commit_batch_->clear_all_first = true;
    commit_batch_->changed_keys.clear();
  }

  map_.clear();
  bytes_used_ = 0;
  observers_.ForAllPtrs(
      [&source](mojom::LevelDBObserver* observer) {
        observer->AllDeleted(source);
      });
  std::move(callback).Run(true);
}

void LevelDBWrapperImpl::Get(const std::vector<uint8_t>& key,
                             GetCallback callback) {
  if (map_state_ != MapLoadingState::LOADED) {
    LoadMap(base::BindOnce(&LevelDBWrapperImpl::Get, base::Unretained(this),
                           key, base::Passed(&callback)));
    return;
  }

  auto found = map_.find(key);
  if (found == map_.end()) {
    std::move(callback).Run(false, std::vector<uint8_t>());
    return;
  }
  std::move(callback).Run(true, found->second);
}

void LevelDBWrapperImpl::GetAll(
    mojom::LevelDBWrapperGetAllCallbackAssociatedPtrInfo complete_callback,
    GetAllCallback callback) {
  if (map_state_ != MapLoadingState::LOADED) {
    LoadMap(base::BindOnce(&LevelDBWrapperImpl::GetAll, base::Unretained(this),
                           base::Passed(&complete_callback),
                           base::Passed(&callback)));
    return;
  }

  std::vector<mojom::KeyValuePtr> all;
  for (const auto& it : map_) {
    mojom::KeyValuePtr kv = mojom::KeyValue::New();
    kv->key = it.first;
    kv->value = it.second;
    all.push_back(std::move(kv));
  }
  std::move(callback).Run(leveldb::mojom::DatabaseError::OK, std::move(all));
  if (complete_callback.is_valid()) {
    mojom::LevelDBWrapperGetAllCallbackAssociatedPtr complete_ptr;
    complete_ptr.Bind(std::move(complete_callback));
    complete_ptr->Complete(true);
  }
}

void LevelDBWrapperImpl::OnConnectionError() {
  if (!bindings_.empty())
    return;
  // If any tasks are waiting for load to complete, delay calling the
  // no_bindings_callback_ until all those tasks have completed.
  if (!on_load_complete_tasks_.empty())
    return;
  delegate_->OnNoBindings();
}

void LevelDBWrapperImpl::LoadMap(base::OnceClosure completion_callback) {
  DCHECK(map_state_ != MapLoadingState::LOADED);
  on_load_complete_tasks_.push_back(std::move(completion_callback));
  if (map_state_ == MapLoadingState::LOADING_FROM_DATABASE ||
      map_state_ == MapLoadingState::LOADING_FROM_FORK)
    return;

  map_state_ = MapLoadingState::LOADING_FROM_DATABASE;

  if (!database_) {
    OnMapLoaded(leveldb::mojom::DatabaseError::IO_ERROR,
                std::vector<leveldb::mojom::KeyValuePtr>());
    return;
  }

  database_->GetPrefixed(prefix_,
                         base::BindOnce(&LevelDBWrapperImpl::OnMapLoaded,
                                        weak_ptr_factory_.GetWeakPtr()));
}

void LevelDBWrapperImpl::OnMapLoaded(
    leveldb::mojom::DatabaseError status,
    std::vector<leveldb::mojom::KeyValuePtr> data) {
  DCHECK(map_state_ != MapLoadingState::LOADED);

  if (data.empty() && status == leveldb::mojom::DatabaseError::OK) {
    delegate_->MigrateData(
        base::BindOnce(&LevelDBWrapperImpl::OnGotMigrationData,
                       weak_ptr_factory_.GetWeakPtr()));
    return;
  }

  map_.clear();
  bytes_used_ = 0;
  for (auto& it : data) {
    DCHECK_GE(it->key.size(), prefix_.size());
    map_[std::vector<uint8_t>(it->key.begin() + prefix_.size(),
                              it->key.end())] = it->value;
    bytes_used_ += it->key.size() - prefix_.size() + it->value.size();
  }
  map_state_ = MapLoadingState::LOADED;

  std::vector<Change> changes = delegate_->FixUpData(map_);
  if (!changes.empty()) {
    DCHECK(database_);
    CreateCommitBatchIfNeeded();
    for (auto& change : changes) {
      auto it = map_.find(change.first);
      if (!change.second) {
        DCHECK(it != map_.end());
        bytes_used_ -= it->first.size() + it->second.size();
        map_.erase(it);
      } else {
        if (it != map_.end()) {
          bytes_used_ -= it->second.size();
          it->second = std::move(*change.second);
          bytes_used_ += it->second.size();
        } else {
          bytes_used_ += change.first.size() + change.second->size();
          map_[change.first] = std::move(*change.second);
        }
      }
      commit_batch_->changed_keys.insert(std::move(change.first));
    }
    CommitChanges();
  }

  // We proceed without using a backing store, nothing will be persisted but the
  // class is functional for the lifetime of the object.
  delegate_->OnMapLoaded(status);
  if (status != leveldb::mojom::DatabaseError::OK)
    database_ = nullptr;

  OnLoadComplete();
}

void LevelDBWrapperImpl::OnGotMigrationData(std::unique_ptr<ValueMap> data) {
  map_ = data ? std::move(*data) : ValueMap();
  bytes_used_ = 0;
  for (const auto& it : map_)
    bytes_used_ += it.first.size() + it.second.size();

  if (database_ && !empty()) {
    CreateCommitBatchIfNeeded();
    for (const auto& it : map_)
      commit_batch_->changed_keys.insert(it.first);
    CommitChanges();
  }

  OnLoadComplete();
}

void LevelDBWrapperImpl::OnLoadComplete() {
  map_state_ = MapLoadingState::LOADED;
  std::vector<base::OnceClosure> tasks;
  on_load_complete_tasks_.swap(tasks);
  for (auto& task : tasks)
    std::move(task).Run();

  destruction_during_load_listeners_.clear();

  // We might need to call the no_bindings_callback_ here if bindings became
  // empty while waiting for load to complete.
  if (bindings_.empty())
    delegate_->OnNoBindings();
}

void LevelDBWrapperImpl::CreateCommitBatchIfNeeded() {
  if (commit_batch_)
    return;

  commit_batch_.reset(new CommitBatch());
  BrowserThread::PostAfterStartupTask(
      FROM_HERE, base::ThreadTaskRunnerHandle::Get(),
      base::BindOnce(&LevelDBWrapperImpl::StartCommitTimer,
                     weak_ptr_factory_.GetWeakPtr()));
}

void LevelDBWrapperImpl::StartCommitTimer() {
  if (!commit_batch_)
    return;

  // Start a timer to commit any changes that accrue in the batch, but only if
  // no commits are currently in flight. In that case the timer will be
  // started after the commits have happened.
  if (commit_batches_in_flight_)
    return;

  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&LevelDBWrapperImpl::CommitChanges,
                     weak_ptr_factory_.GetWeakPtr()),
      ComputeCommitDelay());
}

base::TimeDelta LevelDBWrapperImpl::ComputeCommitDelay() const {
  if (s_aggressive_flushing_enabled_)
    return base::TimeDelta::FromSeconds(1);

  base::TimeDelta elapsed_time = base::TimeTicks::Now() - start_time_;
  base::TimeDelta delay = std::max(
      default_commit_delay_,
      std::max(commit_rate_limiter_.ComputeDelayNeeded(elapsed_time),
               data_rate_limiter_.ComputeDelayNeeded(elapsed_time)));
  UMA_HISTOGRAM_LONG_TIMES("LevelDBWrapper.CommitDelay", delay);
  return delay;
}

void LevelDBWrapperImpl::CommitChanges() {
  // Note: commit_batch_ may be null if ScheduleImmediateCommit was called
  // after a delayed commit task was scheduled.
  if (!commit_batch_)
    return;

  DCHECK(database_);
  DCHECK(map_state_ == MapLoadingState::LOADED);

  commit_rate_limiter_.add_samples(1);

  // Commit all our changes in a single batch.
  std::vector<BatchedOperationPtr> operations = delegate_->PrepareToCommit();
  bool has_changes =
      !operations.empty() || !commit_batch_->changed_keys.empty();

  if (commit_batch_->clear_all_first) {
    BatchedOperationPtr item = BatchedOperation::New();
    item->type = leveldb::mojom::BatchOperationType::DELETE_PREFIXED_KEY;
    item->key = prefix_;
    operations.push_back(std::move(item));
  }
  size_t data_size = 0;
  for (const auto& key : commit_batch_->changed_keys) {
    data_size += key.size();
    BatchedOperationPtr item = BatchedOperation::New();
    item->key.reserve(prefix_.size() + key.size());
    item->key.insert(item->key.end(), prefix_.begin(), prefix_.end());
    item->key.insert(item->key.end(), key.begin(), key.end());
    auto it = map_.find(key);
    if (it == map_.end()) {
      item->type = leveldb::mojom::BatchOperationType::DELETE_KEY;
    } else {
      item->type = leveldb::mojom::BatchOperationType::PUT_KEY;
      item->value = it->second;
      data_size += it->second.size();
    }
    operations.push_back(std::move(item));
  }
  // Schedule the copy, and ignore if |clear_all_first| is specified and there
  // are no changing keys.
  if (commit_batch_->copy_to_prefix_ &&
      (has_changes || !commit_batch_->clear_all_first)) {
    BatchedOperationPtr item = BatchedOperation::New();
    item->type = leveldb::mojom::BatchOperationType::COPY_PREFIXED_KEY;
    item->key = prefix_;
    item->value = std::move(commit_batch_->copy_to_prefix_.value());
    operations.push_back(std::move(item));
  }
  commit_batch_.reset();

  data_rate_limiter_.add_samples(data_size);

  ++commit_batches_in_flight_;

  // TODO(michaeln): Currently there is no guarantee LevelDBDatabaseImpl::Write
  // will run during a clean shutdown. We need that to avoid dataloss.
  database_->Write(std::move(operations),
                   base::BindOnce(&LevelDBWrapperImpl::OnCommitComplete,
                                  weak_ptr_factory_.GetWeakPtr()));
}

void LevelDBWrapperImpl::OnCommitComplete(leveldb::mojom::DatabaseError error) {
  --commit_batches_in_flight_;
  StartCommitTimer();
  delegate_->DidCommit(error);
}

void LevelDBWrapperImpl::DoForkOperation(
    base::WeakPtr<LevelDBWrapperImpl> forked_wrapper) {
  if (!forked_wrapper)
    return;
  // TODO(dmurph): If this commit fails, then the disk could be in an
  // inconsistant state. Ideally all further operations will fail and the code
  // will correctly delete the database?
  if (database_) {
    CreateCommitBatchIfNeeded();
    commit_batch_->copy_to_prefix_ = forked_wrapper->prefix_;
    CommitChanges();
  }

  forked_wrapper->OnForkStateLoaded(database_ != nullptr, map_);
}

void LevelDBWrapperImpl::OnForkStateLoaded(bool database_enabled,
                                           const ValueMap& map) {
  map_ = map;
  map_state_ = MapLoadingState::LOADED;

  if (!database_enabled) {
    database_ = nullptr;
    OnLoadComplete();
    return;
  }

  OnLoadComplete();
}

}  // namespace content
