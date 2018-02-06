// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/discovery/dial/dial_media_sink_service.h"

#include "chrome/browser/browser_process.h"
#include "chrome/browser/media/router/discovery/dial/dial_media_sink_service_impl.h"
#include "chrome/common/media_router/media_source_helper.h"
#include "content/public/browser/browser_context.h"
#include "content/public/common/service_manager_connection.h"
#include "services/service_manager/public/cpp/connector.h"

using content::BrowserThread;

namespace media_router {

namespace {

url::Origin CreateOrigin(const std::string& url) {
  return url::Origin::Create(GURL(url));
}

}  // namespace

DialMediaSinkService::DialMediaSinkService(
    const scoped_refptr<net::URLRequestContextGetter>& request_context)
    : impl_(nullptr, base::OnTaskRunnerDeleter(nullptr)),
      request_context_(request_context),
      weak_ptr_factory_(this) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(request_context_);
}

DialMediaSinkService::~DialMediaSinkService() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void DialMediaSinkService::Start(
    const OnSinksDiscoveredCallback& sink_discovery_cb,
    const OnDialSinkAddedCallback& dial_sink_added_cb) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK(!impl_);

  OnSinksDiscoveredCallback sink_discovery_cb_impl = base::BindRepeating(
      &RunSinksDiscoveredCallbackOnSequence,
      base::SequencedTaskRunnerHandle::Get(),
      base::BindRepeating(&DialMediaSinkService::RunSinksDiscoveredCallback,
                          weak_ptr_factory_.GetWeakPtr(), sink_discovery_cb));

  OnAvailableSinksUpdatedCallback available_sinks_updated_cb_impl =
      base::BindRepeating(
          &RunAvailableSinksUpdatedCallbackOnSequence,
          base::SequencedTaskRunnerHandle::Get(),
          base::BindRepeating(
              &DialMediaSinkService::RunAvailableSinksUpdatedCallback,
              weak_ptr_factory_.GetWeakPtr()));

  impl_ = CreateImpl(sink_discovery_cb_impl, dial_sink_added_cb,
                     available_sinks_updated_cb_impl, request_context_);

  impl_->task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&DialMediaSinkServiceImpl::Start,
                                base::Unretained(impl_.get())));
}

void DialMediaSinkService::OnUserGesture() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  impl_->task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&DialMediaSinkServiceImpl::OnUserGesture,
                                base::Unretained(impl_.get())));
}

void DialMediaSinkService::RegisterOnSinkQueryUpdatedCallback(
    const OnSinkQueryUpdatedCallback& sink_query_updated_cb) {
  DCHECK(!sink_query_updated_cb_);
  sink_query_updated_cb_ = sink_query_updated_cb;
}

void DialMediaSinkService::UnregisterOnSinkQueryUpdatedCallback() {
  sink_query_updated_cb_.Reset();
}

void DialMediaSinkService::StartObservingMediaSinks(
    const std::string& media_source) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::string app_name = AppNameFromDialMediaSource(MediaSource(media_source));
  auto media_sources_it = registered_media_sources_.find(app_name);
  if (media_sources_it == registered_media_sources_.end()) {
    impl_->task_runner()->PostTask(
        FROM_HERE,
        base::BindOnce(
            &DialMediaSinkServiceImpl::StartMonitoringAvailableSinksForApp,
            base::Unretained(impl_.get()), app_name));
    registered_media_sources_[app_name].insert(media_source);
    return;
  }

  auto& media_sources = media_sources_it->second;
  auto media_source_it = media_sources.find(media_source);
  // No op if already observing media source.
  if (media_source_it != media_sources.end())
    return;

  media_sources.insert(media_source);
  if (sink_query_updated_cb_) {
    sink_query_updated_cb_.Run(media_source, cached_available_sinks_[app_name],
                               GetOrigins(app_name));
  }
}

void DialMediaSinkService::StopObservingMediaSinks(
    const std::string& media_source) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::string app_name = AppNameFromDialMediaSource(MediaSource(media_source));
  auto media_sources_it = registered_media_sources_.find(app_name);
  if (media_sources_it == registered_media_sources_.end())
    return;

  impl_->task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(
          &DialMediaSinkServiceImpl::StopMonitoringAvailableSinksForApp,
          base::Unretained(impl_.get()), app_name));
  media_sources_it->second.erase(media_source);
}

std::unique_ptr<DialMediaSinkServiceImpl, base::OnTaskRunnerDeleter>
DialMediaSinkService::CreateImpl(
    const OnSinksDiscoveredCallback& sink_discovery_cb,
    const OnDialSinkAddedCallback& dial_sink_added_cb,
    const OnAvailableSinksUpdatedCallback& available_sinks_updated_cb,
    const scoped_refptr<net::URLRequestContextGetter>& request_context) {
  // Clone the connector so it can be used on the IO thread.
  std::unique_ptr<service_manager::Connector> connector =
      content::ServiceManagerConnection::GetForProcess()
          ->GetConnector()
          ->Clone();

  // Note: The SequencedTaskRunner needs to be IO thread because DialRegistry
  // and URLRequestContextGetter run on IO thread.
  scoped_refptr<base::SequencedTaskRunner> task_runner =
      content::BrowserThread::GetTaskRunnerForThread(
          content::BrowserThread::IO);
  return std::unique_ptr<DialMediaSinkServiceImpl, base::OnTaskRunnerDeleter>(
      new DialMediaSinkServiceImpl(
          std::move(connector), sink_discovery_cb, dial_sink_added_cb,
          available_sinks_updated_cb, request_context, task_runner),
      base::OnTaskRunnerDeleter(task_runner));
}

void DialMediaSinkService::RunSinksDiscoveredCallback(
    const OnSinksDiscoveredCallback& sinks_discovered_cb,
    std::vector<MediaSinkInternal> sinks) {
  sinks_discovered_cb.Run(std::move(sinks));
}

void DialMediaSinkService::RunAvailableSinksUpdatedCallback(
    const std::string& app_name,
    std::vector<MediaSinkInternal> available_sinks) {
  if (!sink_query_updated_cb_)
    return;

  const std::vector<url::Origin>& origins = GetOrigins(app_name);
  for (auto& media_source : registered_media_sources_[app_name])
    sink_query_updated_cb_.Run(media_source, available_sinks, origins);

  cached_available_sinks_[app_name] = available_sinks;
}

std::vector<url::Origin> DialMediaSinkService::GetOrigins(
    const std::string& app_name) {
  static base::flat_map<std::string, std::vector<url::Origin>>
      origin_white_list = {
          {"YouTube",
           {CreateOrigin("https://tv.youtube.com"),
            CreateOrigin("https://tv-green-qa.youtube.com"),
            CreateOrigin("https://tv-release-qa.youtube.com"),
            CreateOrigin("https://web-green-qa.youtube.com"),
            CreateOrigin("https://web-release-qa.youtube.com"),
            CreateOrigin("https://www.youtube.com")}},
          {"Netflix", {CreateOrigin("https://www.netflix.com")}},
          {"Pandora", {CreateOrigin("https://www.pandora.com")}},
          {"Radio", {CreateOrigin("https://www.pandora.com")}},
          {"Hulu", {CreateOrigin("https://www.hulu.com")}},
          {"Vimeo", {CreateOrigin("https://www.vimeo.com")}},
          {"Dailymotion", {CreateOrigin("https://www.dailymotion.com")}},
          {"com.dailymotion", {CreateOrigin("https://www.dailymotion.com")}}};

  auto origins_it = origin_white_list.find(app_name);
  if (origins_it == origin_white_list.end())
    return std::vector<url::Origin>();

  return origins_it->second;
}

}  // namespace media_router
