// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/devtools/protocol/storage_handler.h"

#include <unordered_set>
#include <utility>
#include <vector>

#include "base/lazy_instance.h"
#include "base/strings/string_split.h"
#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/storage_partition.h"
#include "storage/browser/quota/quota_client.h"
#include "storage/browser/quota/quota_manager.h"
#include "storage/common/quota/quota_status_code.h"

namespace content {
namespace protocol {

namespace {
typedef HashMap<String, std::vector<StorageHandler*>> TrackedHandlers;
base::LazyInstance<TrackedHandlers>::Leaky tracked_handlers =
    LAZY_INSTANCE_INITIALIZER;

Storage::StorageType GetTypeName(storage::QuotaClient::ID id) {
  switch (id) {
    case storage::QuotaClient::kFileSystem:
      return Storage::StorageTypeEnum::File_systems;
    case storage::QuotaClient::kDatabase:
      return Storage::StorageTypeEnum::Websql;
    case storage::QuotaClient::kAppcache:
      return Storage::StorageTypeEnum::Appcache;
    case storage::QuotaClient::kIndexedDatabase:
      return Storage::StorageTypeEnum::Indexeddb;
    case storage::QuotaClient::kServiceWorkerCache:
      return Storage::StorageTypeEnum::Cache_storage;
    case storage::QuotaClient::kServiceWorker:
      return Storage::StorageTypeEnum::Service_workers;
    default:
      return Storage::StorageTypeEnum::Other;
  }
}

void ReportUsageAndQuotaDataOnUIThread(
    std::unique_ptr<StorageHandler::GetUsageAndQuotaCallback> callback,
    storage::QuotaStatusCode code,
    int64_t usage,
    int64_t quota,
    base::flat_map<storage::QuotaClient::ID, int64_t> usage_breakdown) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (code != storage::kQuotaStatusOk) {
    return callback->sendFailure(
        Response::Error("Quota information is not available"));
  }

  std::unique_ptr<Array<Storage::UsageForType>> usageList =
      Array<Storage::UsageForType>::create();
  for (const auto& usage : usage_breakdown) {
    std::unique_ptr<Storage::UsageForType> entry =
        Storage::UsageForType::Create()
            .SetStorageType(GetTypeName(usage.first))
            .SetUsage(usage.second)
            .Build();
    usageList->addItem(std::move(entry));
  }
  callback->sendSuccess(usage, quota, std::move(usageList));
}

void GotUsageAndQuotaDataCallback(
    std::unique_ptr<StorageHandler::GetUsageAndQuotaCallback> callback,
    storage::QuotaStatusCode code,
    int64_t usage,
    int64_t quota,
    base::flat_map<storage::QuotaClient::ID, int64_t> usage_breakdown) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                          base::Bind(ReportUsageAndQuotaDataOnUIThread,
                                     base::Passed(std::move(callback)), code,
                                     usage, quota, std::move(usage_breakdown)));
}

void GetUsageAndQuotaOnIOThread(
    storage::QuotaManager* manager,
    const GURL& url,
    std::unique_ptr<StorageHandler::GetUsageAndQuotaCallback> callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  manager->GetUsageAndQuotaWithBreakdown(
      url, storage::kStorageTypeTemporary,
      base::Bind(&GotUsageAndQuotaDataCallback,
                 base::Passed(std::move(callback))));
}
}  // namespace

StorageHandler::StorageHandler()
    : DevToolsDomainHandler(Storage::Metainfo::domainName),
      host_(nullptr) {
}

StorageHandler::~StorageHandler() {
  for (auto& handlers : tracked_handlers.Get()) {
    auto handler =
        std::find(handlers.second.begin(), handlers.second.end(), this);
    if (handler != handlers.second.end())
      handlers.second.erase(handler);
  }
}

void StorageHandler::OnCacheStorageUpdated(const GURL& origin) {
  if (!origin.is_valid())
    return;

  String origin_str = origin.spec();
  auto handlers = tracked_handlers.Get().find(origin_str);
  if (handlers == tracked_handlers.Get().end())
    return;
  for (StorageHandler* handler : handlers->second)
    handler->NotifyCacheStorage();
}

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
  if (set.count(Storage::StorageTypeEnum::Appcache))
    remove_mask |= StoragePartition::REMOVE_DATA_MASK_APPCACHE;
  if (set.count(Storage::StorageTypeEnum::Cookies))
    remove_mask |= StoragePartition::REMOVE_DATA_MASK_COOKIES;
  if (set.count(Storage::StorageTypeEnum::File_systems))
    remove_mask |= StoragePartition::REMOVE_DATA_MASK_FILE_SYSTEMS;
  if (set.count(Storage::StorageTypeEnum::Indexeddb))
    remove_mask |= StoragePartition::REMOVE_DATA_MASK_INDEXEDDB;
  if (set.count(Storage::StorageTypeEnum::Local_storage))
    remove_mask |= StoragePartition::REMOVE_DATA_MASK_LOCAL_STORAGE;
  if (set.count(Storage::StorageTypeEnum::Shader_cache))
    remove_mask |= StoragePartition::REMOVE_DATA_MASK_SHADER_CACHE;
  if (set.count(Storage::StorageTypeEnum::Websql))
    remove_mask |= StoragePartition::REMOVE_DATA_MASK_WEBSQL;
  if (set.count(Storage::StorageTypeEnum::Service_workers))
    remove_mask |= StoragePartition::REMOVE_DATA_MASK_SERVICE_WORKERS;
  if (set.count(Storage::StorageTypeEnum::Cache_storage))
    remove_mask |= StoragePartition::REMOVE_DATA_MASK_CACHE_STORAGE;
  if (set.count(Storage::StorageTypeEnum::All))
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

  GURL origin_url(origin);
  if (!origin_url.is_valid()) {
    return callback->sendFailure(
        Response::Error(origin + " is not a valid URL"));
  }

  storage::QuotaManager* manager =
      host_->GetProcess()->GetStoragePartition()->GetQuotaManager();
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&GetUsageAndQuotaOnIOThread, base::RetainedRef(manager),
                 origin_url, base::Passed(std::move(callback))));
}

void StorageHandler::TrackOrigin(
    const String& origin,
    std::unique_ptr<TrackOriginCallback> callback) {
  GURL origin_url(origin);
  if (!origin_url.is_valid()) {
    return callback->sendFailure(
        Response::Error(origin + " is not a valid URL"));
  }

  auto handlers = tracked_handlers.Get().find(origin_url.spec());
  if (handlers == tracked_handlers.Get().end()) {
    std::vector<StorageHandler*> handlers;
    handlers.push_back(this);
    tracked_handlers.Get()[origin_url.spec()] = std::move(handlers);
  } else if (std::find(handlers->second.begin(), handlers->second.end(),
                       this) == handlers->second.end()) {
    handlers->second.push_back(this);
  }

  if (std::find(tracked_origins_.begin(), tracked_origins_.end(),
                origin_url.spec()) == tracked_origins_.end())
    tracked_origins_.push_back(origin);

  callback->sendSuccess();
}

void StorageHandler::UntrackOrigin(
    const String& origin,
    std::unique_ptr<UntrackOriginCallback> callback) {
  GURL origin_url(origin);
  if (!origin_url.is_valid()) {
    return callback->sendFailure(
        Response::Error(origin + " is not a valid URL"));
  }

  auto handlers = tracked_handlers.Get().find(origin_url.spec());
  if (handlers != tracked_handlers.Get().end()) {
    auto handler =
        std::find(handlers->second.begin(), handlers->second.end(), this);
    if (handler != handlers->second.end())
      handlers->second.erase(handler);
    if (!handlers->second.empty())
      tracked_handlers.Get().erase(handlers);
  }

  auto tracked_origin = std::find(tracked_origins_.begin(),
                                  tracked_origins_.end(), origin_url.spec());
  if (tracked_origin != tracked_origins_.end())
    tracked_origins_.erase(tracked_origin);

  callback->sendSuccess();
}

void StorageHandler::WaitForUpdateCacheStorage(
    std::unique_ptr<WaitForUpdateCacheStorageCallback> callback) {
  if (cache_storage_updated_) {
    callback->sendFailure(Response::Error("Replaced by new callback"));
    cache_storage_updated_ = false;
  } else {
    if (cache_storage_callback_)
      cache_storage_callback_->sendFailure(Response::InternalError());
    cache_storage_callback_ = std::move(callback);
  }
}

void StorageHandler::NotifyCacheStorage() {
  if (cache_storage_callback_) {
    cache_storage_callback_->sendSuccess();
    cache_storage_callback_ = nullptr;
  } else {
    cache_storage_updated_ = true;
  }
}

}  // namespace protocol
}  // namespace content
