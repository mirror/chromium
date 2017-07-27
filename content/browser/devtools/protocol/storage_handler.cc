// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/devtools/protocol/storage_handler.h"

#include <unordered_set>
#include <utility>
#include <vector>

#include "base/strings/string_split.h"
#include "content/browser/cache_storage/cache_storage_context_impl.h"
#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/storage_partition.h"
#include "storage/browser/quota/quota_client.h"
#include "storage/browser/quota/quota_manager.h"
#include "storage/common/quota/quota_status_code.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace content {
namespace protocol {

namespace {
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

void ReportCacheStorageListChangedOnUIThread(
    CacheStorageContext::ObservationType type,
    const GURL& origin,
    base::Callback<void(const String&)> callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (type != CacheStorageContext::ObservationType::CACHE_LIST_CHANGED)
    return;
  callback.Run(origin.spec());
}

void GotCacheStorageListChanged(base::Callback<void(const String&)> callback,
                                CacheStorageContext::ObservationType type,
                                const GURL& origin) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(ReportCacheStorageListChangedOnUIThread, type, origin,
                 base::Passed(std::move(callback))));
}

void AddObserverOnIOThread(CacheStorageContext* context,
                           const GURL& origin,
                           int64_t id,
                           base::Callback<void(const String&)> callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  context->AddObserver(origin, id,
                       base::Bind(&GotCacheStorageListChanged,
                                  base::Passed(std::move(callback))));
}

void RemoveObserverOnIOThread(CacheStorageContext* context,
                              const GURL& origin,
                              int64_t id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  context->RemoveObserver(origin, id);
}

void RemoveAllObserversOnIOThread(
    CacheStorageContext* context,
    std::vector<std::pair<url::Origin, int64_t>> to_remove) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  for (std::pair<url::Origin, int64_t>& observer : to_remove)
    context->RemoveObserver(observer.first.GetURL(), observer.second);
}
}  // namespace

StorageHandler::StorageHandler()
    : DevToolsDomainHandler(Storage::Metainfo::domainName),
      host_(nullptr),
      ptr_factory_(this) {}

StorageHandler::~StorageHandler() {
  if (!observers_.empty()) {
    CacheStorageContext* context =
        host_->GetProcess()->GetStoragePartition()->GetCacheStorageContext();

    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::Bind(&RemoveAllObserversOnIOThread, base::RetainedRef(context),
                   std::move(observers_)));
  }
}

void StorageHandler::Wire(UberDispatcher* dispatcher) {
  frontend_.reset(new Storage::Frontend(dispatcher->channel()));
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
  if (!host_)
    return callback->sendFailure(Response::InternalError());

  GURL origin_url(origin);
  if (!origin_url.is_valid()) {
    return callback->sendFailure(
        Response::Error(origin + " is not a valid URL"));
  }

  auto already_tracking =
      std::find_if(observers_.begin(), observers_.end(),
                   [origin_url](const std::pair<url::Origin, int64_t>& origin) {
                     return origin.first.GetURL() == origin_url;
                   });
  if (already_tracking != observers_.end())
    return callback->sendFailure(Response::Error("Already tracking origin."));

  CacheStorageContext* context =
      host_->GetProcess()->GetStoragePartition()->GetCacheStorageContext();

  int64_t id = CacheStorageContextImpl::GenerateObserverID();
  observers_.push_back(std::make_pair(url::Origin(origin_url), id));

  base::Callback<void(const String&)> notify_callback =
      base::Bind(&StorageHandler::NotifyCacheStorageListChanged,
                 ptr_factory_.GetWeakPtr());

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&AddObserverOnIOThread, base::RetainedRef(context), origin_url,
                 id, base::Passed(std::move(notify_callback))));
  callback->sendSuccess();
}

void StorageHandler::UntrackOrigin(
    const String& origin,
    std::unique_ptr<UntrackOriginCallback> callback) {
  if (!host_)
    return callback->sendFailure(Response::InternalError());

  GURL origin_url(origin);
  if (!origin_url.is_valid()) {
    return callback->sendFailure(
        Response::Error(origin + " is not a valid URL"));
  }

  auto observer =
      std::find_if(observers_.begin(), observers_.end(),
                   [origin_url](const std::pair<url::Origin, int64_t>& origin) {
                     return origin.first.GetURL() == origin_url;
                   });
  if (observer == observers_.end())
    return callback->sendFailure(
        Response::Error("Origin is not being tracked."));
  int64_t id = observer->second;
  observers_.erase(observer);

  CacheStorageContext* context =
      host_->GetProcess()->GetStoragePartition()->GetCacheStorageContext();

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&RemoveObserverOnIOThread, base::RetainedRef(context),
                 origin_url, id));
  callback->sendSuccess();
}

void StorageHandler::NotifyCacheStorageListChanged(const String& origin) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!frontend_)
    frontend_->CacheStorageListUpdate(origin);
}

}  // namespace protocol
}  // namespace content
