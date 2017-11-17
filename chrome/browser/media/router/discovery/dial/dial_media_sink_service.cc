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
  DCHECK(!impl_);
}

void DialMediaSinkService::Start(
    const OnSinksDiscoveredCallback& sink_discovery_cb,
    const OnDialSinkAddedCallback& dial_sink_added_cb) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (impl_)
    return;

  OnDialSinkAddedCallback dial_cb_on_sequence =
      dial_sink_added_cb.is_null()
          ? OnDialSinkAddedCallback()
          : base::BindRepeating(&OnDialSinkAddedOnSequence, dial_sink_added_cb);

  impl_ = CreateImpl(
      base::BindRepeating(&OnSinksDiscoveredOnSequence, sink_discovery_cb),
      dial_cb_on_sequence, request_context_);

  impl_->task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&DialMediaSinkServiceImpl::Start,
                                base::Unretained(impl_.get())));
}

void DialMediaSinkService::Stop() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (!impl_)
    return;

  impl_->task_runner()->PostTask(FROM_HERE,
                                 base::BindOnce(&DialMediaSinkServiceImpl::Stop,
                                                base::Unretained(impl_.get())));
  impl_->task_runner()->DeleteSoon(FROM_HERE, impl_.release());
}

void DialMediaSinkService::ForceSinkDiscoveryCallback() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!impl_)
    return;

  impl_->task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&DialMediaSinkServiceImpl::ForceSinkDiscoveryCallback,
                     base::Unretained(impl_.get())));
}

void DialMediaSinkService::OnUserGesture() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!impl_)
    return;

  impl_->task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&DialMediaSinkServiceImpl::OnUserGesture,
                                base::Unretained(impl_.get())));
}

std::unique_ptr<DialMediaSinkServiceImpl> DialMediaSinkService::CreateImpl(
    const OnSinksDiscoveredCallback& sink_discovery_cb,
    const OnDialSinkAddedCallback& dial_sink_added_cb,
    const scoped_refptr<net::URLRequestContextGetter>& request_context) {
  return std::make_unique<DialMediaSinkServiceImpl>(
      sink_discovery_cb, dial_sink_added_cb, request_context);
}

}  // namespace media_router
