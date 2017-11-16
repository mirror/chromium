// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/discovery/dial/dial_media_sink_service.h"

#include "chrome/browser/media/router/discovery/dial/dial_media_sink_service_impl.h"
#include "content/public/browser/browser_context.h"

using content::BrowserThread;

namespace media_router {

namespace {

// These functions are invoked by DialMediaSinkServiceImpl on its
// SequencedTaskRunner.
void OnSinksDiscoveredOnSequence(
    const OnSinksDiscoveredCallback& sink_discovery_cb,
    std::vector<MediaSinkInternal> sinks) {
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::BindOnce(sink_discovery_cb, std::move(sinks)));
}

void OnDialSinkAddedOnSequence(
    const OnDialSinkAddedCallback& dial_sink_added_cb,
    const MediaSinkInternal& sink) {
  content::BrowserThread::PostTask(content::BrowserThread::UI, FROM_HERE,
                                   base::BindOnce(dial_sink_added_cb, sink));
}

}  // namespace

DialMediaSinkService::DialMediaSinkService(content::BrowserContext* context) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  auto* profile = Profile::FromBrowserContext(context);
  request_context_ = profile->GetRequestContext();
  DCHECK(request_context_);
}

DialMediaSinkService::~DialMediaSinkService() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(!dial_media_sink_service_);
}

void DialMediaSinkService::Start(
    const OnSinksDiscoveredCallback& sink_discovery_cb,
    const OnDialSinkAddedCallback& dial_sink_added_cb) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (dial_media_sink_service_)
    return;

  OnDialSinkAddedCallback cb_on_sequence =
      dial_sink_added_cb.is_null()
          ? OnDialSinkAddedCallback()
          : base::BindRepeating(&OnDialSinkAddedOnSequence, dial_sink_added_cb);

  dial_media_sink_service_ = std::make_unique<DialMediaSinkServiceImpl>(
      base::BindRepeating(&OnSinksDiscoveredOnSequence, sink_discovery_cb),
      cb_on_sequence, request_context_);

  dial_media_sink_service_->task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&DialMediaSinkServiceImpl::Start,
                     base::Unretained(dial_media_sink_service_.get())));
}

void DialMediaSinkService::Stop() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (!dial_media_sink_service_)
    return;

  dial_media_sink_service_->task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&DialMediaSinkServiceImpl::Stop,
                     base::Unretained(dial_media_sink_service_.get())));
  dial_media_sink_service_->task_runner()->DeleteSoon(
      FROM_HERE, dial_media_sink_service_.release());
}

void DialMediaSinkService::ForceSinkDiscoveryCallback() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!dial_media_sink_service_)
    return;

  dial_media_sink_service_->task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&DialMediaSinkServiceImpl::ForceSinkDiscoveryCallback,
                     base::Unretained(dial_media_sink_service_.get())));
}

void DialMediaSinkService::OnUserGesture() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!dial_media_sink_service_)
    return;

  dial_media_sink_service_->task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&DialMediaSinkServiceImpl::OnUserGesture,
                     base::Unretained(dial_media_sink_service_.get())));
}

void DialMediaSinkService::SetDialMediaSinkServiceForTest(
    std::unique_ptr<DialMediaSinkServiceImpl> dial_media_sink_service) {
  DCHECK(dial_media_sink_service);
  DCHECK(!dial_media_sink_service_);
  dial_media_sink_service_ = std::move(dial_media_sink_service);
}

}  // namespace media_router
