// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/discovery/dial/dial_media_sink_service_proxy.h"

#include "chrome/browser/media/router/discovery/dial/dial_media_sink_service_impl.h"
#include "chrome/common/media_router/media_source_helper.h"
#include "content/public/browser/browser_context.h"

using content::BrowserThread;

namespace media_router {

DialMediaSinkServiceProxy::DialMediaSinkServiceProxy(
    const MediaSinkService::OnSinksDiscoveredCallback& callback,
    content::BrowserContext* context)
    : MediaSinkService(callback), observer_(nullptr) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  auto* profile = Profile::FromBrowserContext(context);
  request_context_ = profile->GetRequestContext();
  DCHECK(request_context_);
}

DialMediaSinkServiceProxy::~DialMediaSinkServiceProxy() = default;

void DialMediaSinkServiceProxy::Start() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::BindOnce(&DialMediaSinkServiceProxy::StartOnIOThread, this));
}

void DialMediaSinkServiceProxy::Stop() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::BindOnce(&DialMediaSinkServiceProxy::StopOnIOThread, this));
}

void DialMediaSinkServiceProxy::ForceSinkDiscoveryCallback() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!dial_media_sink_service_)
    return;

  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::BindOnce(&DialMediaSinkServiceImpl::ForceSinkDiscoveryCallback,
                     dial_media_sink_service_->AsWeakPtr()));
}

void DialMediaSinkServiceProxy::OnUserGesture() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!dial_media_sink_service_)
    return;

  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::BindOnce(&DialMediaSinkServiceImpl::OnUserGesture,
                     dial_media_sink_service_->AsWeakPtr()));
}

void DialMediaSinkServiceProxy::SetObserver(
    DialMediaSinkServiceObserver* observer) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  observer_ = observer;
}

void DialMediaSinkServiceProxy::ClearObserver() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  observer_ = nullptr;
}

void DialMediaSinkServiceProxy::SetDialMediaSinkServiceForTest(
    std::unique_ptr<DialMediaSinkServiceImpl> dial_media_sink_service) {
  DCHECK(dial_media_sink_service);
  DCHECK(!dial_media_sink_service_);
  dial_media_sink_service_ = std::move(dial_media_sink_service);
}

bool DialMediaSinkServiceProxy::RegisterMediaSinksObserver(
    MediaSinksObserver* observer) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(observer);

  std::string app_name = AppNameFromMediaSource(observer->source());
  if (app_name.empty()) {
    DVLOG(1) << "Fail to get app name from DIAL source: "
             << observer->source().id();
    return false;
  }

  auto& observers = app_name_sink_observers_map_[app_name];
  if (!observers.HasObserver(observer))
    observers.AddObserver(observer);

  if (!dial_media_sink_service_)
    return true;

  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::BindOnce(
          &DialMediaSinkServiceImpl::StartMonitoringAvailableSinksForApp,
          dial_media_sink_service_->AsWeakPtr(), app_name));
  return true;
}

void DialMediaSinkServiceProxy::UnregisterMediaSinksObserver(
    MediaSinksObserver* observer) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(observer);

  std::string app_name = AppNameFromMediaSource(observer->source());
  auto it = app_name_sink_observers_map_.find(app_name);
  if (it != app_name_sink_observers_map_.end())
    it->second.RemoveObserver(observer);

  if (!dial_media_sink_service_)
    return;

  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::BindOnce(
          &DialMediaSinkServiceImpl::StopMonitoringAvailableSinksForApp,
          dial_media_sink_service_->AsWeakPtr(), app_name));
}

void DialMediaSinkServiceProxy::StartOnIOThread() {
  if (!dial_media_sink_service_) {
    // Need to explicitly delete |dial_media_sink_service_| outside dtor to
    // avoid circular dependency.
    dial_media_sink_service_ = base::MakeUnique<DialMediaSinkServiceImpl>(
        base::Bind(&DialMediaSinkServiceProxy::OnSinksDiscoveredOnIOThread,
                   this),
        base::Bind(
            &DialMediaSinkServiceProxy::OnAvailableSinksUpdatedOnIOThread,
            this),
        request_context_.get());
    dial_media_sink_service_->SetObserver(observer_);
  }

  dial_media_sink_service_->Start();
  for (const auto& app_name_it : app_name_sink_observers_map_) {
    dial_media_sink_service_->StartMonitoringAvailableSinksForApp(
        app_name_it.first);
  }
}

void DialMediaSinkServiceProxy::StopOnIOThread() {
  if (!dial_media_sink_service_)
    return;

  dial_media_sink_service_->Stop();
  dial_media_sink_service_->ClearObserver();
  dial_media_sink_service_.reset();
}

void DialMediaSinkServiceProxy::OnSinksDiscoveredOnIOThread(
    std::vector<MediaSinkInternal> sinks) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::BindOnce(sink_discovery_callback_, std::move(sinks)));
}

void DialMediaSinkServiceProxy::OnAvailableSinksUpdatedOnIOThread(
    const std::string& app_name,
    const std::vector<MediaSink>& available_sinks) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::BindOnce(&DialMediaSinkServiceProxy::OnAvailableSinksUpdated, this,
                     app_name, available_sinks));
}

void DialMediaSinkServiceProxy::OnAvailableSinksUpdated(
    const std::string& app_name,
    const std::vector<MediaSink>& available_sinks) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DVLOG(2) << "OnAvailableSinksUpdated [app name]: " << app_name
           << " [size]: " << available_sinks.size();

  const auto& sink_observers_it = app_name_sink_observers_map_.find(app_name);
  if (sink_observers_it == app_name_sink_observers_map_.end())
    return;

  for (auto& sink_observer : sink_observers_it->second) {
    sink_observer.OnSinksUpdated(available_sinks, std::vector<url::Origin>());
  }
}

}  // namespace media_router
