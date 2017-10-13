// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/background_fetch/background_fetch_delegate_impl.h"

#include <utility>

#include "base/bind.h"
#include "base/guid.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "chrome/browser/download/download_service_factory.h"
#include "chrome/browser/offline_items_collection/offline_content_aggregator_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/download/public/download_params.h"
#include "components/download/public/download_service.h"
#include "components/offline_items_collection/core/offline_content_aggregator.h"
#include "components/offline_items_collection/core/offline_item.h"
#include "content/public/browser/background_fetch_response.h"
#include "content/public/browser/browser_thread.h"

BackgroundFetchDelegateImpl::BackgroundFetchDelegateImpl(Profile* profile)
    : download_service_(
          DownloadServiceFactory::GetInstance()->GetForBrowserContext(profile)),
      offline_content_aggregator_(
          offline_items_collection::OfflineContentAggregatorFactory::
              GetForBrowserContext(profile)),
      weak_ptr_factory_(this) {
  offline_content_aggregator_->RegisterProvider("background_fetch", this);
}

BackgroundFetchDelegateImpl::~BackgroundFetchDelegateImpl() {}

void BackgroundFetchDelegateImpl::Shutdown() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (client()) {
    client()->OnDelegateShutdown();
  }
}

BackgroundFetchDelegateImpl::JobDetails::JobDetails(JobDetails&&) = default;

BackgroundFetchDelegateImpl::JobDetails::JobDetails(const std::string& job_id,
                                                    const std::string& title,
                                                    const url::Origin& origin,
                                                    int completed_parts,
                                                    int total_parts)
    : title(title),
      origin(origin),
      completed_parts(completed_parts),
      total_parts(total_parts),
      offline_item(
          offline_items_collection::ContentId("background_fetch", job_id)) {
  UpdateOfflineItem();
}

BackgroundFetchDelegateImpl::JobDetails::~JobDetails() {}

void BackgroundFetchDelegateImpl::JobDetails::UpdateOfflineItem() {
  if (total_parts > 0) {
    offline_item.progress.value = completed_parts;
    offline_item.progress.max = total_parts;
    offline_item.progress.unit =
        offline_items_collection::OfflineItemProgressUnit::PERCENTAGE;
  }
  if (title.empty()) {
    offline_item.title = origin.Serialize();
  } else {
    offline_item.title = base::StringPrintf("%s (%s)", title.c_str(),
                                            origin.Serialize().c_str());
  }
  offline_item.description = "More detailed item: " + offline_item.title;
  offline_item.is_transient = true;
  offline_item.state =
      (completed_parts == total_parts)
          ? offline_items_collection::OfflineItemState::COMPLETE
          : offline_items_collection::OfflineItemState::IN_PROGRESS;
}

void BackgroundFetchDelegateImpl::CreateDownloadJob(
    const std::string& job_id,
    const std::string& title,
    const url::Origin& origin,
    int completed_parts,
    int total_parts,
    const std::vector<std::string>& current_guids) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  DCHECK(job_details_map_.find(job_id) == job_details_map_.end());

  auto emplace_result = job_details_map_.emplace(
      job_id, JobDetails(job_id, title, origin, completed_parts, total_parts));

  const JobDetails& details = emplace_result.first->second;

  for (auto& guid : current_guids) {
    DCHECK(file_download_job_map_.find(guid) == file_download_job_map_.end());
    file_download_job_map_.emplace(guid, job_id);
  }

  for (auto* observer : observers_) {
    observer->OnItemsAdded({details.offline_item});
  }
}

void BackgroundFetchDelegateImpl::DownloadUrl(
    const std::string& job_id,
    const std::string& guid,
    const std::string& method,
    const GURL& url,
    const net::NetworkTrafficAnnotationTag& traffic_annotation,
    const net::HttpRequestHeaders& headers) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  DCHECK(job_details_map_.find(job_id) != job_details_map_.end());
  DCHECK(file_download_job_map_.find(guid) == file_download_job_map_.end());

  file_download_job_map_.emplace(guid, job_id);

  download::DownloadParams params;
  params.guid = guid;
  params.client = download::DownloadClient::BACKGROUND_FETCH;
  params.request_params.method = method;
  params.request_params.url = url;
  params.request_params.request_headers = headers;
  params.callback = base::Bind(&BackgroundFetchDelegateImpl::OnDownloadReceived,
                               weak_ptr_factory_.GetWeakPtr());
  params.traffic_annotation =
      net::MutableNetworkTrafficAnnotationTag(traffic_annotation);

  download_service_->StartDownload(params);
}

void BackgroundFetchDelegateImpl::OnDownloadStarted(
    const std::string& guid,
    std::unique_ptr<content::BackgroundFetchResponse> response) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (client())
    client()->OnDownloadStarted(guid, std::move(response));
}

void BackgroundFetchDelegateImpl::OnDownloadUpdated(const std::string& guid,
                                                    uint64_t bytes_downloaded) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (client())
    client()->OnDownloadUpdated(guid, bytes_downloaded);
}

void BackgroundFetchDelegateImpl::OnDownloadFailed(
    const std::string& guid,
    download::Client::FailureReason reason) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  using FailureReason = content::BackgroundFetchResult::FailureReason;
  FailureReason failure_reason;

  switch (reason) {
    case download::Client::FailureReason::NETWORK:
      failure_reason = FailureReason::NETWORK;
      break;
    case download::Client::FailureReason::TIMEDOUT:
      failure_reason = FailureReason::TIMEDOUT;
      break;
    case download::Client::FailureReason::UNKNOWN:
      failure_reason = FailureReason::UNKNOWN;
      break;

    case download::Client::FailureReason::ABORTED:
    case download::Client::FailureReason::CANCELLED:
      // The client cancelled or aborted it so no need to notify it.
      return;
    default:
      NOTREACHED();
      return;
  }

  if (client()) {
    client()->OnDownloadComplete(
        guid, std::make_unique<content::BackgroundFetchResult>(
                  base::Time::Now(), failure_reason));
  }
}

void BackgroundFetchDelegateImpl::OnDownloadSucceeded(
    const std::string& guid,
    const base::FilePath& path,
    uint64_t size) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  const std::string& job_id = file_download_job_map_[guid];
  JobDetails& job_details = job_details_map_.find(job_id)->second;
  ++job_details.completed_parts;
  job_details.UpdateOfflineItem();

  for (auto* observer : observers_)
    observer->OnItemUpdated(job_details.offline_item);

  if (client()) {
    client()->OnDownloadComplete(
        guid, std::make_unique<content::BackgroundFetchResult>(
                  base::Time::Now(), path, size));
  }

  file_download_job_map_.erase(guid);
}

void BackgroundFetchDelegateImpl::OnDownloadReceived(
    const std::string& guid,
    download::DownloadParams::StartResult result) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  using StartResult = download::DownloadParams::StartResult;
  switch (result) {
    case StartResult::ACCEPTED:
      // Nothing to do.
      break;
    case StartResult::BACKOFF:
      // TODO(delphick): try again later?
      // TODO(delphick): Due to a bug at the moment, this happens all the time
      // because successful downloads are not removed, so don't NOTREACHED.
      break;
    case StartResult::UNEXPECTED_CLIENT:
      // This really should never happen since we're supplying the
      // DownloadClient.
      NOTREACHED();
    case StartResult::UNEXPECTED_GUID:
      // TODO(delphick): try again with a different GUID.
      NOTREACHED();
    case StartResult::CLIENT_CANCELLED:
      // TODO(delphick): do we need to do anything here, since we will have
      // cancelled it?
      break;
    case StartResult::INTERNAL_ERROR:
      // TODO(delphick): We need to handle this gracefully.
      NOTREACHED();
    case StartResult::COUNT:
      NOTREACHED();
  }
}

bool BackgroundFetchDelegateImpl::AreItemsAvailable() {
  return true;
}

void BackgroundFetchDelegateImpl::OpenItem(
    const offline_items_collection::ContentId& id) {
  NOTIMPLEMENTED();
}

void BackgroundFetchDelegateImpl::RemoveItem(
    const offline_items_collection::ContentId& id) {
  NOTIMPLEMENTED();
}

void BackgroundFetchDelegateImpl::CancelDownload(
    const offline_items_collection::ContentId& id) {
  // TODO(delphick): consider the data structures here.
  for (auto& entry : file_download_job_map_) {
    if (entry.second == id.id) {
      const std::string& download_guid = entry.first;
      download_service_->CancelDownload(download_guid);
      if (client()) {
        client()->OnDownloadComplete(
            download_guid,
            std::make_unique<content::BackgroundFetchResult>(
                base::Time::Now(),
                content::BackgroundFetchResult::FailureReason::CANCELLED));
      }
    }
  }
}

void BackgroundFetchDelegateImpl::PauseDownload(
    const offline_items_collection::ContentId& id) {
  for (auto& entry : file_download_job_map_) {
    if (entry.second == id.id) {
      const std::string& download_guid = entry.first;
      download_service_->PauseDownload(download_guid);
    }
  }

  // TODO(delphick): Mark overall download job as paused so that future
  // downloads are not started until resume. (Initially not a worry because only
  // one download will be scheduled at a time).
}

void BackgroundFetchDelegateImpl::ResumeDownload(
    const offline_items_collection::ContentId& id,
    bool has_user_gesture) {
  for (auto& entry : file_download_job_map_) {
    if (entry.second == id.id) {
      const std::string& download_guid = entry.first;
      download_service_->ResumeDownload(download_guid);
    }
  }

  // TODO(delphick): Start new downloads that weren't started because of pause.
}

const offline_items_collection::OfflineItem*
BackgroundFetchDelegateImpl::GetItemById(
    const offline_items_collection::ContentId& id) {
  auto it = job_details_map_.find(id.id);
  return (it != job_details_map_.end()) ? &it->second.offline_item : nullptr;
}

BackgroundFetchDelegateImpl::OfflineItemList
BackgroundFetchDelegateImpl::GetAllItems() {
  OfflineItemList item_list;
  for (auto& entry : job_details_map_)
    item_list.push_back(entry.second.offline_item);
  return item_list;
}

void BackgroundFetchDelegateImpl::GetVisualsForItem(
    const offline_items_collection::ContentId& id,
    const VisualsCallback& callback) {
  // GetVisualsForItem mustn't be called directly since offline_items_collection
  // is not re-entrant and it must be called even if there are no visuals.
  // TODO(delphick): Call with an image when that becomes available.
  content::BrowserThread::PostTask(content::BrowserThread::UI, FROM_HERE,
                                   base::BindOnce(callback, id, nullptr));
}

void BackgroundFetchDelegateImpl::AddObserver(Observer* observer) {
  observers_.insert(observer);
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::BindOnce(
          [](Observer* observer, BackgroundFetchDelegateImpl* provider) {
            observer->OnItemsAvailable(provider);
          },
          observer, this));
}

void BackgroundFetchDelegateImpl::RemoveObserver(Observer* observer) {
  observers_.erase(observer);
}
