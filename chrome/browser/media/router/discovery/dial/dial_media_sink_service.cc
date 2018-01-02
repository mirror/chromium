// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/discovery/dial/dial_media_sink_service.h"

#include "chrome/browser/media/router/discovery/dial/dial_media_sink_service_impl.h"
#include "chrome/common/media_router/media_source_helper.h"
#include "content/public/browser/browser_context.h"
#include "content/public/common/service_manager_connection.h"
#include "services/service_manager/public/cpp/connector.h"

using content::BrowserThread;

namespace media_router {

namespace {

void RunDialSinkAddedCallbackOnSequence(
    const OnDialSinkAddedCallback& dial_sink_added_cb,
    const scoped_refptr<base::SequencedTaskRunner>& task_runner,
    const MediaSinkInternal& dial_sink) {
  task_runner->PostTask(FROM_HERE,
                        base::BindOnce(dial_sink_added_cb, dial_sink));
}

void RunAvailableSinksUpdatedCallbackOnSequence(
    const OnAvailableSinksUpdatedCallback& available_sink_updated_cb,
    const scoped_refptr<base::SequencedTaskRunner>& task_runner,
    const std::string& app_name,
    const std::vector<MediaSinkInternal>& available_sinks) {
  task_runner->PostTask(FROM_HERE, base::BindOnce(available_sink_updated_cb,
                                                  app_name, available_sinks));
}

}  // namespace

DialMediaSinkService::DialMediaSinkService(content::BrowserContext* context)
    : impl_(nullptr, base::OnTaskRunnerDeleter(nullptr)) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  auto* profile = Profile::FromBrowserContext(context);
  request_context_ = profile->GetRequestContext();
  DCHECK(request_context_);
}

DialMediaSinkService::~DialMediaSinkService() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void DialMediaSinkService::Start(
    const OnSinksDiscoveredCallback& sink_discovery_cb,
    const OnDialSinkAddedCallback& dial_sink_added_cb,
    const OnAvailableSinksUpdatedCallback& available_sinks_updated_cb,
    const scoped_refptr<base::SequencedTaskRunner>&
        dial_sink_added_cb_sequence) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK(!impl_);

  OnSinksDiscoveredCallback sink_discovery_cb_impl = base::BindRepeating(
      &RunSinksDiscoveredCallbackOnSequence,
      base::SequencedTaskRunnerHandle::Get(), sink_discovery_cb);
  OnDialSinkAddedCallback dial_sink_added_cb_impl;
  if (dial_sink_added_cb) {
    dial_sink_added_cb_impl =
        base::BindRepeating(&RunDialSinkAddedCallbackOnSequence,
                            dial_sink_added_cb, dial_sink_added_cb_sequence);
  }
  OnAvailableSinksUpdatedCallback available_sinks_updated_cb_impl =
      base::BindRepeating(&RunAvailableSinksUpdatedCallbackOnSequence,
                          available_sinks_updated_cb,
                          base::SequencedTaskRunnerHandle::Get());

  impl_ = CreateImpl(sink_discovery_cb_impl, dial_sink_added_cb_impl,
                     available_sinks_updated_cb_impl, request_context_);

  impl_->task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&DialMediaSinkServiceImpl::Start,
                                base::Unretained(impl_.get())));
}

void DialMediaSinkService::ForceSinkDiscoveryCallback() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  impl_->task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(&DialMediaSinkServiceImpl::ForceSinkDiscoveryCallback,
                     base::Unretained(impl_.get())));
}

void DialMediaSinkService::OnUserGesture() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  impl_->task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&DialMediaSinkServiceImpl::OnUserGesture,
                                base::Unretained(impl_.get())));
}

void DialMediaSinkService::StartMonitoringAvailableSinksForApp(
    const std::string& app_name) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  impl_->task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(
          &DialMediaSinkServiceImpl::StartMonitoringAvailableSinksForApp,
          base::Unretained(impl_.get()), app_name));
}

void DialMediaSinkService::StopMonitoringAvailableSinksForApp(
    const std::string& app_name) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  impl_->task_runner()->PostTask(
      FROM_HERE,
      base::BindOnce(
          &DialMediaSinkServiceImpl::StopMonitoringAvailableSinksForApp,
          base::Unretained(impl_.get()), app_name));
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

}  // namespace media_router
