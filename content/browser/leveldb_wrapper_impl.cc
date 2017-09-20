// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/leveldb_wrapper_impl.h"

#include "base/bind.h"
#include "base/metrics/histogram_macros.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/memory_dump_manager.h"
#include "base/trace_event/process_memory_dump.h"
#include "build/build_config.h"
#include "components/leveldb/public/cpp/util.h"
#include "content/public/browser/browser_thread.h"

namespace content {

void LevelDBWrapperImpl::Delegate::MigrateData(
    base::OnceCallback<void(std::unique_ptr<ValueMap>)> callback) {
  std::move(callback).Run(nullptr);
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
      load_state_(LOAD_STATE_UNLOADED),
#if defined(OS_ANDROID)
      desired_load_state_(database ? LOAD_STATE_KEYS_ONLY
                                   : LOAD_STATE_KEYS_AND_VALUES),
#else
      desired_load_state_(LOAD_STATE_KEYS_AND_VALUES),
#endif
      storage_used_(0),
      max_size_(max_size),
      memory_used_(0),
      start_time_(base::TimeTicks::Now()),
      default_commit_delay_(default_commit_delay),
      data_rate_limiter_(max_bytes_per_hour, base::TimeDelta::FromHours(1)),
      commit_rate_limiter_(max_commits_per_hour, base::TimeDelta::FromHours(1)),
      weak_ptr_factory_(this) {
  bindings_.set_connection_error_handler(base::Bind(
      &LevelDBWrapperImpl::OnConnectionError, base::Unretained(this)));
}

LevelDBWrapperImpl::~LevelDBWrapperImpl() {
  if (commit_batch_)
    CommitChanges();
}

void LevelDBWrapperImpl::Bind(mojom::LevelDBWrapperRequest request) {
  bindings_.AddBinding(this, std::move(request));
#if defined(OS_ANDROID)
  // If the number of bindings is more than 1, then the |old_client_value| sent
  // by the clients need not be valid due to races. So, cache the values in the
  // servcie.
  if (bindings_.size() > 1)
    SetCacheMode(/*only_keys=*/false);
#endif
}

void LevelDBWrapperImpl::EnableAggressiveCommitDelay() {
  s_aggressive_flushing_enabled_ = true;
}

void LevelDBWrapperImpl::ScheduleImmediateCommit() {
  if (!on_load_complete_tasks_.empty()) {
    LoadMap(base::Bind(&LevelDBWrapperImpl::ScheduleImmediateCommit,
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
  if (load_state_ == LOAD_STATE_UNLOADED)
    return;

  const char* system_allocator_name =
      base::trace_event::MemoryDumpManager::GetInstance()
          ->system_allocator_pool_name();
  if (commit_batch_) {
    size_t data_size = 0;
    for (const auto& iter : commit_batch_->changed_values) {
      data_size +=
          iter.first.size() + (iter.second ? iter.second.value().size() : 0);
    }
    auto* commit_batch_mad = pmd->CreateAllocatorDump(name + "/commit_batch");
    commit_batch_mad->AddScalar(
        base::trace_event::MemoryAllocatorDump::kNameSize,
        base::trace_event::MemoryAllocatorDump::kUnitsBytes, data_size);
    if (system_allocator_name)
      pmd->AddSuballocation(commit_batch_mad->guid(), system_allocator_name);
  }

  // Do not add storage map usage if less than 1KB.
  if (memory_used_ < 1024)
    return;

  auto* map_mad = pmd->CreateAllocatorDump(name + "/storage_map");
  map_mad->AddScalar(base::trace_event::MemoryAllocatorDump::kNameSize,
                     base::trace_event::MemoryAllocatorDump::kUnitsBytes,
                     memory_used_);
  map_mad->AddString(
      "load_state", "",
      load_state_ == LOAD_STATE_KEYS_ONLY ? "keys_only" : "keys_and_values");
  if (system_allocator_name)
    pmd->AddSuballocation(map_mad->guid(), system_allocator_name);
}

void LevelDBWrapperImpl::PurgeMemory() {
  if (load_state_ == LOAD_STATE_UNLOADED ||  // We're not using any memory.
      commit_batch_ ||  // We leave things alone with changes pending.
      !database_) {  // Don't purge anything if we're not backed by a database.
    return;
  }

  load_state_ = LOAD_STATE_UNLOADED;
  keys_values_map_.reset();
  keys_only_map_.reset();
}

void LevelDBWrapperImpl::SetCacheModeForTesting(bool only_keys) {
  SetCacheMode(only_keys);
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
  if (IsMapReloadNeeded()) {
    LoadMap(base::Bind(&LevelDBWrapperImpl::Put, base::Unretained(this), key,
                       value, client_old_value, source,
                       base::Passed(&callback)));
    return;
  }

  size_t old_item_size = 0;
  size_t old_item_memory = 0;
  size_t new_item_memory = 0;
  base::Optional<std::vector<uint8_t>> old_value;
  if (load_state_ == LOAD_STATE_KEYS_ONLY) {
    KeysOnlyMap::const_iterator found = keys_only_map_->find(key);
    if (found != keys_only_map_->end()) {
      if (client_old_value.value() == value) {
        std::move(callback).Run(true);  // Key already has this value.
        return;
      }
      DCHECK_EQ(found->second, client_old_value.value().size());
      old_value = client_old_value.value();
      old_item_size = key.size() + found->second;
      old_item_memory = key.size() + sizeof(size_t);
    }
    new_item_memory = key.size() + sizeof(size_t);
  } else {
    ValueMap::iterator found = keys_values_map_->find(key);
    if (found != keys_values_map_->end()) {
      if (found->second == value) {
        std::move(callback).Run(true);  // Key already has this value.
        return;
      }
      old_value = std::move(found->second);
      old_item_size = key.size() + old_value.value().size();
      old_item_memory = old_item_size;
    }
    new_item_memory = key.size() + value.size();
  }
  size_t new_item_size = key.size() + value.size();
  size_t new_storage_used = storage_used_ - old_item_size + new_item_size;
  size_t new_memory_used = memory_used_ - old_item_memory + new_item_memory;

  // Only check quota if the size is increasing, this allows
  // shrinking changes to pre-existing maps that are over budget.
  if (new_item_size > old_item_size && new_storage_used > max_size_) {
    std::move(callback).Run(false);
    return;
  }

  if (database_) {
    CreateCommitBatchIfNeeded();
    // Values will be taken from |keys_values_map_| if loaded before committing.
    if (load_state_ == LOAD_STATE_KEYS_ONLY)
      commit_batch_->changed_values[key] = value;
    else
      commit_batch_->changed_values[key];
  }

  if (load_state_ == LOAD_STATE_KEYS_ONLY)
    (*keys_only_map_)[key] = value.size();
  else
    (*keys_values_map_)[key] = value;

  storage_used_ = new_storage_used;
  memory_used_ = new_memory_used;
  if (!old_value) {
    // We added a new key/value pair.
    observers_.ForAllPtrs(
        [&key, &value, &source](mojom::LevelDBObserver* observer) {
          observer->KeyAdded(key, value, source);
        });
  } else {
    // We changed the value for an existing key.
    observers_.ForAllPtrs(
        [&key, &value, &source, &old_value](mojom::LevelDBObserver* observer) {
          observer->KeyChanged(key, value, old_value.value(), source);
        });
  }
  std::move(callback).Run(true);
}

void LevelDBWrapperImpl::Delete(
    const std::vector<uint8_t>& key,
    const base::Optional<std::vector<uint8_t>>& client_old_value,
    const std::string& source,
    DeleteCallback callback) {
  if (IsMapReloadNeeded()) {
    LoadMap(base::Bind(&LevelDBWrapperImpl::Delete, base::Unretained(this), key,
                       client_old_value, source, base::Passed(&callback)));
    return;
  }

  std::vector<uint8_t> old_value;
  if (load_state_ == LOAD_STATE_KEYS_ONLY) {
    KeysOnlyMap::const_iterator found = keys_only_map_->find(key);
    if (found == keys_only_map_->end()) {
      std::move(callback).Run(true);
      return;
    }
    DCHECK_EQ(found->second, client_old_value.value().size());
    old_value = client_old_value.value();
    keys_only_map_->erase(found);
    memory_used_ -= key.size() + sizeof(size_t);
  } else {
    ValueMap::iterator found = keys_values_map_->find(key);
    if (found == keys_values_map_->end()) {
      std::move(callback).Run(true);
      return;
    }
    old_value.swap(found->second);
    keys_values_map_->erase(found);
    memory_used_ -= key.size() + old_value.size();
  }

  if (database_) {
    CreateCommitBatchIfNeeded();
    commit_batch_->changed_values[key];
  }

  storage_used_ -= key.size() + old_value.size();
  observers_.ForAllPtrs(
      [&key, &source, &old_value](mojom::LevelDBObserver* observer) {
        observer->KeyDeleted(key, old_value, source);
      });
  std::move(callback).Run(true);
}

void LevelDBWrapperImpl::DeleteAll(const std::string& source,
                                   DeleteAllCallback callback) {
  if (IsMapReloadNeeded()) {
    LoadMap(base::Bind(&LevelDBWrapperImpl::DeleteAll, base::Unretained(this),
                       source, base::Passed(&callback)));
    return;
  }

  if ((load_state_ == LOAD_STATE_KEYS_ONLY && keys_only_map_->empty()) ||
      (load_state_ == LOAD_STATE_KEYS_AND_VALUES &&
       keys_values_map_->empty())) {
    std::move(callback).Run(true);
    return;
  }

  if (database_) {
    CreateCommitBatchIfNeeded();
    commit_batch_->clear_all_first = true;
    commit_batch_->changed_values.clear();
  }

  if (load_state_ == LOAD_STATE_KEYS_ONLY)
    keys_only_map_->clear();
  else
    keys_values_map_->clear();
  storage_used_ = 0;
  memory_used_ = 0;
  observers_.ForAllPtrs(
      [&source](mojom::LevelDBObserver* observer) {
        observer->AllDeleted(source);
      });
  std::move(callback).Run(true);
}

void LevelDBWrapperImpl::Get(const std::vector<uint8_t>& key,
                             GetCallback callback) {
  DCHECK_EQ(LOAD_STATE_KEYS_AND_VALUES, desired_load_state_);
  if (IsMapReloadNeeded()) {
    LoadMap(base::Bind(&LevelDBWrapperImpl::Get, base::Unretained(this), key,
                       base::Passed(&callback)));
    return;
  }

  auto found = keys_values_map_->find(key);
  if (found == keys_values_map_->end()) {
    std::move(callback).Run(false, std::vector<uint8_t>());
    return;
  }
  std::move(callback).Run(true, found->second);
}

void LevelDBWrapperImpl::GetAll(
    mojom::LevelDBWrapperGetAllCallbackAssociatedPtrInfo complete_callback,
    GetAllCallback callback) {
  if (load_state_ != LOAD_STATE_KEYS_AND_VALUES) {
    LoadMap(base::Bind(&LevelDBWrapperImpl::GetAll, base::Unretained(this),
                       base::Passed(&complete_callback),
                       base::Passed(&callback)));
    return;
  }

  std::vector<mojom::KeyValuePtr> all;
  for (const auto& it : (*keys_values_map_)) {
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

void LevelDBWrapperImpl::SetCacheMode(bool only_keys) {
  LoadState new_desired_state =
      only_keys ? LOAD_STATE_KEYS_ONLY : LOAD_STATE_KEYS_AND_VALUES;
  if (!database_ || new_desired_state == desired_load_state_)
    return;
  desired_load_state_ = new_desired_state;
  // If the keys only map is loaded and desired state needs values, no point
  // keeping around the map since the next change would require reload. On the
  // other hand if only keys are desired, the keys and values map can still be
  // used. The map is unloaded on next purge or when the commit is done.
  if (load_state_ == LOAD_STATE_KEYS_ONLY &&
      desired_load_state_ == LOAD_STATE_KEYS_AND_VALUES &&
      !keys_only_map_->empty()) {
    UnloadMapIfDesired();
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

void LevelDBWrapperImpl::LoadMap(const base::Closure& completion_callback) {
  DCHECK(!keys_values_map_);
  // Current commit batch needs to be applied before re-loading the map. The
  // re-load of map occurs only when GetAll() or CacheMode is set to keys and
  // values and the keys only map is already loaded. In this case commit batch
  // needs to be committed before reading the database.
  if (load_state_ == LOAD_STATE_KEYS_ONLY) {
    DCHECK(on_load_complete_tasks_.empty());
    DCHECK(database_);
    if (commit_batch_)
      CommitChanges();
    // Make sure the keys only map is not used when on load tasks are in queue.
    // The changes to the wrapper will be queued to on load tasks.
    load_state_ = LOAD_STATE_UNLOADED;
    keys_only_map_ = nullptr;
  }

  on_load_complete_tasks_.push_back(completion_callback);
  if (on_load_complete_tasks_.size() > 1)
    return;

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
  DCHECK(!keys_values_map_);

  if (data.empty() && status == leveldb::mojom::DatabaseError::OK) {
    delegate_->MigrateData(
        base::BindOnce(&LevelDBWrapperImpl::OnGotMigrationData,
                       weak_ptr_factory_.GetWeakPtr()));
    return;
  }

  keys_values_map_.reset(new ValueMap);
  load_state_ = LOAD_STATE_KEYS_AND_VALUES;
  storage_used_ = 0;
  for (auto& it : data) {
    DCHECK_GE(it->key.size(), prefix_.size());
    (*keys_values_map_)[std::vector<uint8_t>(it->key.begin() + prefix_.size(),
                                             it->key.end())] = it->value;
    storage_used_ += it->key.size() - prefix_.size() + it->value.size();
  }
  memory_used_ = storage_used_;

  // We proceed without using a backing store, nothing will be persisted but the
  // class is functional for the lifetime of the object.
  delegate_->OnMapLoaded(status);
  if (status != leveldb::mojom::DatabaseError::OK)
    database_ = nullptr;

  OnLoadComplete();
}

void LevelDBWrapperImpl::OnGotMigrationData(std::unique_ptr<ValueMap> data) {
  keys_values_map_ = data ? std::move(data) : base::MakeUnique<ValueMap>();
  load_state_ = LOAD_STATE_KEYS_AND_VALUES;
  storage_used_ = 0;
  for (const auto& it : *keys_values_map_)
    storage_used_ += it.first.size() + it.second.size();
  memory_used_ = storage_used_;

  if (database_ && !empty()) {
    CreateCommitBatchIfNeeded();
    // Values will be taken from |keys_values_map_| if loaded before committing.
    for (const auto& it : *keys_values_map_)
      commit_batch_->changed_values[it.first];
    CommitChanges();
  }

  OnLoadComplete();
}

void LevelDBWrapperImpl::OnLoadComplete() {
  std::vector<base::Closure> tasks;
  on_load_complete_tasks_.swap(tasks);
  for (auto& task : tasks)
    task.Run();

  if (desired_load_state_ == LOAD_STATE_KEYS_ONLY)
    UnloadMapIfDesired();
  DCHECK(!IsMapReloadNeeded());

  // We might need to call the no_bindings_callback_ here if bindings became
  // empty while waiting for load to complete.
  if (bindings_.empty())
    delegate_->OnNoBindings();
}

void LevelDBWrapperImpl::CreateCommitBatchIfNeeded() {
  if (commit_batch_)
    return;
  DCHECK(database_);

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
  DCHECK(database_);
  DCHECK(!IsMapReloadNeeded());
  if (!commit_batch_)
    return;

  commit_rate_limiter_.add_samples(1);

  // Commit all our changes in a single batch.
  std::vector<leveldb::mojom::BatchedOperationPtr> operations =
      delegate_->PrepareToCommit();
  if (commit_batch_->clear_all_first) {
    leveldb::mojom::BatchedOperationPtr item =
        leveldb::mojom::BatchedOperation::New();
    item->type = leveldb::mojom::BatchOperationType::DELETE_PREFIXED_KEY;
    item->key = prefix_;
    operations.push_back(std::move(item));
  }
  size_t data_size = 0;
  for (const auto& it : commit_batch_->changed_values) {
    const auto& key = it.first;
    data_size += key.size();
    leveldb::mojom::BatchedOperationPtr item =
        leveldb::mojom::BatchedOperation::New();
    item->key.reserve(prefix_.size() + key.size());
    item->key.insert(item->key.end(), prefix_.begin(), prefix_.end());
    item->key.insert(item->key.end(), key.begin(), key.end());
    const std::vector<uint8_t>* value = nullptr;
    if (load_state_ == LOAD_STATE_KEYS_AND_VALUES) {
      auto kv_it = keys_values_map_->find(key);
      if (kv_it != keys_values_map_->end())
        value = &kv_it->second;
    } else {
      if (it.second)
        value = &it.second.value();
    }
    if (!value) {
      item->type = leveldb::mojom::BatchOperationType::DELETE_KEY;
    } else {
      item->type = leveldb::mojom::BatchOperationType::PUT_KEY;
      item->value = *value;
      data_size += value->size();
    }
    operations.push_back(std::move(item));
  }
  commit_batch_.reset();

  data_rate_limiter_.add_samples(data_size);

  ++commit_batches_in_flight_;

  // TODO(michaeln): Currently there is no guarantee LevelDBDatabaseImp::Write
  // will run during a clean shutdown. We need that to avoid dataloss.
  database_->Write(std::move(operations),
                   base::BindOnce(&LevelDBWrapperImpl::OnCommitComplete,
                                  weak_ptr_factory_.GetWeakPtr()));
}

void LevelDBWrapperImpl::OnCommitComplete(leveldb::mojom::DatabaseError error) {
  --commit_batches_in_flight_;
  StartCommitTimer();
  delegate_->DidCommit(error);

  // When the desired load state is changed, the unload of map is deferred
  // when there are uncommitted changes. So, try again after committing.
  UnloadMapIfDesired();
}

void LevelDBWrapperImpl::UnloadMapIfDesired() {
  if (load_state_ == LOAD_STATE_UNLOADED || load_state_ == desired_load_state_)
    return;

  // Do not clear the map if there are uncommitted changes since the commit
  // batch might not have the values populated.
  if (!database_ || commit_batch_ || commit_batches_in_flight_)
    return;
  if (desired_load_state_ == LOAD_STATE_KEYS_ONLY) {
    keys_only_map_.reset(new KeysOnlyMap);
    auto it = keys_values_map_->begin();
    memory_used_ = 0;
    while (it != keys_values_map_->end()) {
      memory_used_ += it->first.size() + sizeof(size_t);
      keys_only_map_->insert(
          std::make_pair(std::move(it->first), it->second.size()));
      keys_values_map_->erase(it);
      it = keys_values_map_->begin();
    }
    keys_values_map_ = nullptr;
    load_state_ = LOAD_STATE_KEYS_ONLY;
  } else {
    if (keys_only_map_->empty()) {
      keys_values_map_.reset(new ValueMap);
      load_state_ = LOAD_STATE_KEYS_AND_VALUES;
    } else {
      load_state_ = LOAD_STATE_UNLOADED;
    }
    keys_only_map_ = nullptr;
  }
}

bool LevelDBWrapperImpl::IsMapReloadNeeded() const {
  return load_state_ < desired_load_state_ || !on_load_complete_tasks_.empty();
}

}  // namespace content
