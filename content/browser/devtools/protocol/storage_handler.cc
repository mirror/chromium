// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/devtools/protocol/storage_handler.h"

#include <string>
#include <unordered_set>
#include <vector>

#include "base/strings/string_split.h"
#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/storage_partition.h"
#include "storage/browser/quota/quota_manager.h"
#include "storage/common/quota/quota_status_code.h"

namespace content {
namespace protocol {

namespace {
static const char kAppCache[] = "appcache";
static const char kCookies[] = "cookies";
static const char kFileSystems[] = "filesystems";
static const char kIndexedDB[] = "indexeddb";
static const char kLocalStorage[] = "local_storage";
static const char kShaderCache[] = "shader_cache";
static const char kWebSQL[] = "websql";
static const char kServiceWorkers[] = "service_workers";
static const char kCacheStorage[] = "cache_storage";
static const char kAll[] = "all";

class UsageAndQuotaDataCollector
    : public base::RefCounted<UsageAndQuotaDataCollector> {
 public:
  using UsageCallbackPtr =
      std::unique_ptr<StorageHandler::GetUsageAndQuotaCallback>;
  // using Storage::QuotasAndUsage;

  explicit UsageAndQuotaDataCollector(UsageCallbackPtr&& callback)
      : callback_(std::move(callback)) {}

  void TempDataCallback(storage::QuotaStatusCode code,
                        int64_t usage,
                        int64_t quota) {
    if (!callback_)
      return;
    if (code != storage::kQuotaStatusOk) {
      callback_->sendFailure(
          Response::Error("Quota information is not available"));
      callback_.reset();
      return;
    }
    temp_usage_ = usage;
    temp_quota_ = quota;
  }

  void PersistentDataCallback(storage::QuotaStatusCode code,
                              int64_t usage,
                              int64_t quota) {
    if (!callback_)
      return;
    if (code != storage::kQuotaStatusOk) {
      callback_->sendFailure(
          Response::Error("Quota information is not available"));
      callback_.reset();
      return;
    }
    perstent_usage_ = usage;
    persistent_quota_ = quota;
  }

 private:
  friend class base::RefCounted<UsageAndQuotaDataCollector>;
  virtual ~UsageAndQuotaDataCollector() {
    if (!callback_)
      return;
    callback_->sendSuccess(Storage::QuotasAndUsage::Create()
                               .SetPersistentStorageQuota(persistent_quota_)
                               .SetPersistentStorageUsage(perstent_usage_)
                               .SetTemporaryStorageQuota(temp_quota_)
                               .SetTemporaryStorageUsage(temp_usage_)
                               .Build());
  }

  UsageCallbackPtr callback_;

  int64_t temp_usage_;
  int64_t perstent_usage_;
  int64_t temp_quota_;
  int64_t persistent_quota_;
};
}

StorageHandler::StorageHandler()
    : DevToolsDomainHandler(Storage::Metainfo::domainName),
      host_(nullptr) {
}

StorageHandler::~StorageHandler() = default;

void StorageHandler::Wire(UberDispatcher* dispatcher) {
  Storage::Dispatcher::wire(dispatcher, this);
}

void StorageHandler::SetRenderFrameHost(RenderFrameHostImpl* host) {
  host_ = host;
}

Response StorageHandler::ClearDataForOrigin(
    const std::string& origin,
    const std::string& storage_types) {
  if (!host_)
    return Response::InternalError();

  StoragePartition* partition = host_->GetProcess()->GetStoragePartition();
  std::vector<std::string> types = base::SplitString(
      storage_types, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  std::unordered_set<std::string> set(types.begin(), types.end());
  uint32_t remove_mask = 0;
  if (set.count(kAppCache))
    remove_mask |= StoragePartition::REMOVE_DATA_MASK_APPCACHE;
  if (set.count(kCookies))
    remove_mask |= StoragePartition::REMOVE_DATA_MASK_COOKIES;
  if (set.count(kFileSystems))
    remove_mask |= StoragePartition::REMOVE_DATA_MASK_FILE_SYSTEMS;
  if (set.count(kIndexedDB))
    remove_mask |= StoragePartition::REMOVE_DATA_MASK_INDEXEDDB;
  if (set.count(kLocalStorage))
    remove_mask |= StoragePartition::REMOVE_DATA_MASK_LOCAL_STORAGE;
  if (set.count(kShaderCache))
    remove_mask |= StoragePartition::REMOVE_DATA_MASK_SHADER_CACHE;
  if (set.count(kWebSQL))
    remove_mask |= StoragePartition::REMOVE_DATA_MASK_WEBSQL;
  if (set.count(kServiceWorkers))
    remove_mask |= StoragePartition::REMOVE_DATA_MASK_SERVICE_WORKERS;
  if (set.count(kCacheStorage))
    remove_mask |= StoragePartition::REMOVE_DATA_MASK_CACHE_STORAGE;
  if (set.count(kAll))
    remove_mask |= StoragePartition::REMOVE_DATA_MASK_ALL;

  if (!remove_mask)
    return Response::InvalidParams("No valid storage type specified");

  partition->ClearDataForOrigin(
      remove_mask,
      StoragePartition::QUOTA_MANAGED_STORAGE_MASK_ALL,
      GURL(origin),
      partition->GetURLRequestContext(),
      base::Bind(&base::DoNothing));
  return Response::OK();
}

void StorageHandler::GetUsageAndQuota(
    const String& origin,
    std::unique_ptr<GetUsageAndQuotaCallback> callback) {
  if (!host_)
    return callback->sendFailure(Response::InternalError());

  scoped_refptr<UsageAndQuotaDataCollector> collector(
      new UsageAndQuotaDataCollector(std::move(callback)));

  storage::QuotaManager* manager =
      host_->GetProcess()->GetStoragePartition()->GetQuotaManager();
  GURL origin_url(origin);
  manager->GetUsageAndQuotaForWebApps(
      origin_url, storage::kStorageTypeTemporary,
      base::Bind(&UsageAndQuotaDataCollector::TempDataCallback, collector));
  manager->GetUsageAndQuotaForWebApps(
      origin_url, storage::kStorageTypePersistent,
      base::Bind(&UsageAndQuotaDataCollector::PersistentDataCallback,
                 collector));
}

}  // namespace protocol
}  // namespace content
