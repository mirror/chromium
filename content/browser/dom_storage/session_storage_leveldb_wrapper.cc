// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/dom_storage/session_storage_leveldb_wrapper.h"

#include "base/bind.h"
#include "base/metrics/histogram_macros.h"
#include "base/sys_info.h"
#include "components/leveldb/public/cpp/util.h"
#include "content/browser/dom_storage/session_storage_context_mojo.h"
#include "content/common/dom_storage/dom_storage_types.h"
#include "third_party/leveldatabase/env_chromium.h"

namespace content {
namespace {
static constexpr const char kLevelDBRefCountedMapSource[] =
    "LevelDBRefCountedMap";
void NoOpSuccess(bool success) {}
void NoOpLevelDB(leveldb::mojom::DatabaseError) {}
}  // namespace

SessionStorageLevelDBWrapper::SessionStorageLevelDBWrapper(
    SessionStorageContextMojo* context)
    : shared_map_(new LevelDBRefCountedMap(context)),
      context_(context),
      binding_(this),
      ptr_factory_(this) {}

SessionStorageLevelDBWrapper::SessionStorageLevelDBWrapper(
    SessionStorageContextMojo* context,
    scoped_refptr<LevelDBRefCountedMap> map)
    : shared_map_(std::move(map)),
      context_(context),
      binding_(this),
      ptr_factory_(this) {}

SessionStorageLevelDBWrapper::~SessionStorageLevelDBWrapper() {}

void SessionStorageLevelDBWrapper::Bind(mojom::LevelDBWrapperRequest request) {
  binding_.Bind(std::move(request));
  shared_map_->AddBindingReference();
  binding_.set_connection_error_handler(
      base::BindOnce(&SessionStorageLevelDBWrapper::OnConnectionError,
                     base::Unretained(this)));
}

void SessionStorageLevelDBWrapper::OnDBConnectionComplete(
    leveldb::mojom::LevelDBDatabase* database,
    std::vector<uint8_t> map_prefix) {
  LOG(INFO) << "OnDBConnectionComplete";
  if (!shared_map_->IsConnected())
    shared_map_->Connect(database, std::move(map_prefix));
}

base::OnceClosure SessionStorageLevelDBWrapper::CreateStartClosure() {
  return base::BindOnce(&SessionStorageLevelDBWrapper::StartOperations,
                        ptr_factory_.GetWeakPtr());
}

void SessionStorageLevelDBWrapper::StartOperations() {
  is_connected_ = true;
  std::vector<base::OnceClosure> on_load_complete_tasks;
  std::swap(on_load_complete_tasks_, on_load_complete_tasks);
  for (auto& task : on_load_complete_tasks)
    std::move(task).Run();
}

std::unique_ptr<SessionStorageLevelDBWrapper>
SessionStorageLevelDBWrapper::Clone() {
  return std::make_unique<SessionStorageLevelDBWrapper>(context_, shared_map_);
}

void SessionStorageLevelDBWrapper::OnConnectionError() {
  LOG(INFO) << "OnConnectionError";
  shared_map_->RemoveBindingReference();
}

// LevelDBWrapper:
void SessionStorageLevelDBWrapper::AddObserver(mojom::LevelDBObserverAssociatedPtrInfo observer) {
  if (!is_connected_) {
    LOG(INFO) << "adding observer later";
    on_load_complete_tasks_.push_back(
        base::BindOnce(&SessionStorageLevelDBWrapper::AddObserver,
                       base::Unretained(this), std::move(observer)));
    return;
  }

  LOG(INFO) << "AddObserver";
  mojom::LevelDBObserverAssociatedPtr observer_ptr;
  observer_ptr.Bind(std::move(observer));
  if (shared_map_->level_db_wrapper()->cache_mode() ==
      LevelDBWrapperImpl::CacheMode::KEYS_AND_VALUES)
    observer_ptr->ShouldSendOldValueOnMutations(false);
  mojo::PtrId ptr_id =
      shared_map_->level_db_wrapper()->AddObserver(std::move(observer_ptr));
  observer_ptrs_.push_back(ptr_id);
}
void SessionStorageLevelDBWrapper::Put(
    const std::vector<uint8_t>& key,
    const std::vector<uint8_t>& value,
    const base::Optional<std::vector<uint8_t>>& client_old_value,
    const std::string& source,
    PutCallback callback) {
  if (!is_connected_) {
    LOG(INFO) << "putting later";
    on_load_complete_tasks_.push_back(base::BindOnce(
        &SessionStorageLevelDBWrapper::Put, base::Unretained(this), key, value,
        client_old_value, source, std::move(callback)));
    return;
  }
  LOG(INFO) << "Put";
  if (!shared_map_->HasOneRef())
    ForkToNewMap();
  shared_map_->level_db_wrapper()->Put(key, value, client_old_value, source,
                                       std::move(callback));
}
void SessionStorageLevelDBWrapper::Delete(const std::vector<uint8_t>& key,
            const base::Optional<std::vector<uint8_t>>& client_old_value,
            const std::string& source,
            DeleteCallback callback) {
  if (!is_connected_) {
    LOG(INFO) << "deleting later";
    on_load_complete_tasks_.push_back(base::BindOnce(
        &SessionStorageLevelDBWrapper::Delete, base::Unretained(this), key,
        client_old_value, source, std::move(callback)));
    return;
  }
  LOG(INFO) << "Delete";
  if (!shared_map_->HasOneRef())
    ForkToNewMap();
  shared_map_->level_db_wrapper()->Delete(key, client_old_value, source,
                                          std::move(callback));
}
void SessionStorageLevelDBWrapper::DeleteAll(const std::string& source, DeleteAllCallback callback) {
  if (!is_connected_) {
    LOG(INFO) << "deleting all later";
    on_load_complete_tasks_.push_back(
        base::BindOnce(&SessionStorageLevelDBWrapper::DeleteAll,
                       base::Unretained(this), source, std::move(callback)));
    return;
  }
  LOG(INFO) << "DeleteAll";
  if (!shared_map_->HasOneRef())
    ForkToNewMap();
  shared_map_->level_db_wrapper()->DeleteAll(source, std::move(callback));
}
void SessionStorageLevelDBWrapper::Get(const std::vector<uint8_t>& key, GetCallback callback) {
  if (!is_connected_) {
    LOG(INFO) << "getting later";
    on_load_complete_tasks_.push_back(
        base::BindOnce(&SessionStorageLevelDBWrapper::Get,
                       base::Unretained(this), key, std::move(callback)));
    return;
  }
  LOG(INFO) << "Get";
  shared_map_->level_db_wrapper()->Get(key, std::move(callback));
}
void SessionStorageLevelDBWrapper::GetAll(
    mojom::LevelDBWrapperGetAllCallbackAssociatedPtrInfo complete_callback,
    GetAllCallback callback) {
  if (!is_connected_) {
    LOG(INFO) << "getting all later";
    on_load_complete_tasks_.push_back(base::BindOnce(
        &SessionStorageLevelDBWrapper::GetAll, base::Unretained(this),
        std::move(complete_callback), std::move(callback)));
    return;
  }
  LOG(INFO) << "GetAll";
  shared_map_->level_db_wrapper()->GetAll(std::move(complete_callback),
                                          std::move(callback));
}


SessionStorageLevelDBWrapper::LevelDBRefCountedMap::LevelDBRefCountedMap(
    SessionStorageContextMojo* context)
    : context_(context) {}

// Constructor for forking.
SessionStorageLevelDBWrapper::LevelDBRefCountedMap::LevelDBRefCountedMap(
    std::vector<uint8_t> map_prefix,
    LevelDBRefCountedMap* forking_from)
    : context_(forking_from->context_),
      wrapper_impl_(forking_from->level_db_wrapper()->ForkToNewPrefix(
          std::move(map_prefix),
          this,
          GetOptions())),
      level_db_wrapper_ptr_(wrapper_impl_.get()) {
  DCHECK(forking_from->IsConnected());
}

SessionStorageLevelDBWrapper::LevelDBRefCountedMap::~LevelDBRefCountedMap() {
  if (wrapper_impl_) {
    wrapper_impl_->DeleteAll(kLevelDBRefCountedMapSource,
                             base::BindOnce(&NoOpSuccess));
    context_->OnDataMapDestruction(wrapper_impl_->prefix());
  }
}

void SessionStorageLevelDBWrapper::LevelDBRefCountedMap::Connect(
    leveldb::mojom::LevelDBDatabase* database,
    std::vector<uint8_t> map_prefix) {
  wrapper_impl_ = std::make_unique<LevelDBWrapperImpl>(
      database, std::move(map_prefix), this, GetOptions());
  level_db_wrapper_ptr_ = wrapper_impl_.get();
}

void SessionStorageLevelDBWrapper::LevelDBRefCountedMap::
    RemoveBindingReference() {
  DCHECK_GT(binding_count_, 0);
  --binding_count_;
  if (binding_count_ > 0)
    return;
  // Don't delete ourselves, but do schedule an immediate commit. Possible
  // deletion will happen under memory pressure or when another localstorage
  // area is opened.
  level_db_wrapper()->ScheduleImmediateCommit();
}

std::vector<leveldb::mojom::BatchedOperationPtr>
SessionStorageLevelDBWrapper::LevelDBRefCountedMap::PrepareToCommit() {
  return std::vector<leveldb::mojom::BatchedOperationPtr>();
}

void SessionStorageLevelDBWrapper::LevelDBRefCountedMap::DidCommit(
    leveldb::mojom::DatabaseError error) {
  UMA_HISTOGRAM_ENUMERATION("SessionStorageContext.CommitResult",
                            leveldb::GetLevelDBStatusUMAValue(error),
                            leveldb_env::LEVELDB_STATUS_MAX);
  context_->OnCommitResult(error);
}

LevelDBWrapperImpl::Options
SessionStorageLevelDBWrapper::LevelDBRefCountedMap::GetOptions() {
  // Delay for a moment after a value is set in anticipation
  // of other values being set, so changes are batched.
  constexpr const base::TimeDelta kCommitDefaultDelaySecs =
      base::TimeDelta::FromSeconds(5);

  // To avoid excessive IO we apply limits to the amount of data being
  // written and the frequency of writes.
  const int kMaxBytesPerHour = kPerStorageAreaQuota;
  const int kMaxCommitsPerHour = 60;

  LevelDBWrapperImpl::Options options;
  options.max_size = kPerStorageAreaQuota + kPerStorageAreaOverQuotaAllowance;
  options.default_commit_delay = kCommitDefaultDelaySecs;
  options.max_bytes_per_hour = kMaxBytesPerHour;
  options.max_commits_per_hour = kMaxCommitsPerHour;
#if defined(OS_ANDROID)
  options.cache_mode = LevelDBWrapperImpl::CacheMode::KEYS_ONLY_WHEN_POSSIBLE;
#else
  options.cache_mode = LevelDBWrapperImpl::CacheMode::KEYS_AND_VALUES;
  if (base::SysInfo::IsLowEndDevice()) {
    options.cache_mode = LevelDBWrapperImpl::CacheMode::KEYS_ONLY_WHEN_POSSIBLE;
  }
#endif
  return options;
}

void SessionStorageLevelDBWrapper::ForkToNewMap() {
  LOG(INFO) << "ForkToNewMap";
  std::vector<mojom::LevelDBObserverAssociatedPtr> ptrs_to_move;
  for (const mojo::PtrId& ptr_id : observer_ptrs_) {
    DCHECK(shared_map_->level_db_wrapper()->HasObserver(ptr_id));
    ptrs_to_move.push_back(
        shared_map_->level_db_wrapper()->RemoveObserver(ptr_id));
  }
  observer_ptrs_.clear();
  shared_map_->RemoveBindingReference();
  shared_map_ = base::MakeRefCounted<LevelDBRefCountedMap>(
      context_->GetAndWriteNextMapPrefix(), shared_map_.get());
  shared_map_->AddBindingReference();
  DCHECK(shared_map_->HasOneRef());

  for (auto& observer_ptr : ptrs_to_move) {
    observer_ptrs_.push_back(
        shared_map_->level_db_wrapper()->AddObserver(std::move(observer_ptr)));
  }
}

} /* namespace content */
