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
    const OnAvailableSinksUpdatedCallback& available_sinks_updated_cb) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK(!impl_);

  OnSinksDiscoveredCallback sink_discovery_cb_impl = base::BindRepeating(
      &RunSinksDiscoveredCallbackOnSequence,
      base::SequencedTaskRunnerHandle::Get(),
      base::BindRepeating(&DialMediaSinkService::RunSinksDiscoveredCallback,
                          weak_ptr_factory_.GetWeakPtr(), sink_discovery_cb));

  OnAvailableSinksUpdatedCallback available_sinks_updated_cb_impl =
      base::BindRepeating(
          &DialMediaSinkService::RunAvailableSinksUpdatedCallback,
          weak_ptr_factory_.GetWeakPtr(), available_sinks_updated_cb);

  impl_ = CreateImpl(sink_discovery_cb_impl, dial_sink_added_cb,
                     available_sinks_updated_cb_impl);

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

void DialMediaSinkService::RunSinksDiscoveredCallback(
    const OnSinksDiscoveredCallback& sinks_discovered_cb,
    std::vector<MediaSinkInternal> sinks) {
  sinks_discovered_cb.Run(std::move(sinks));
}

}  // namespace media_router
