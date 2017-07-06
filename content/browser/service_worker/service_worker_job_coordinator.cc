// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_job_coordinator.h"

#include <stddef.h>

#include <memory>
#include <utility>

#include "base/memory/ptr_util.h"
#include "content/browser/service_worker/service_worker_register_job_base.h"

namespace content {

ServiceWorkerJobCoordinator::JobQueue::JobQueue() = default;

ServiceWorkerJobCoordinator::JobQueue::JobQueue(JobQueue&&) = default;

ServiceWorkerJobCoordinator::JobQueue::~JobQueue() {
  DCHECK(jobs_.empty()) << "Destroying JobQueue with " << jobs_.size()
                        << " unfinished jobs";
}

ServiceWorkerRegisterJobBase* ServiceWorkerJobCoordinator::JobQueue::Push(
    std::unique_ptr<ServiceWorkerRegisterJobBase> job) {
  if (jobs_.empty()) {
    job->Start();
    jobs_.push_back(std::move(job));
  } else if (!job->Equals(jobs_.back().get())) {
    jobs_.push_back(std::move(job));
  }
  // Note we are releasing 'job' here in case neither of the two if() statements
  // above were true.

  DCHECK(!jobs_.empty());
  return jobs_.back().get();
}

void ServiceWorkerJobCoordinator::JobQueue::Pop(
    ServiceWorkerRegisterJobBase* job) {
  DCHECK(job == jobs_.front().get());
  jobs_.pop_front();
  if (!jobs_.empty())
    jobs_.front()->Start();
}

void ServiceWorkerJobCoordinator::JobQueue::AbortAll() {
  for (const auto& job : jobs_)
    job->Abort();
  jobs_.clear();
}

void ServiceWorkerJobCoordinator::JobQueue::ClearForShutdown() {
  jobs_.clear();
}

ServiceWorkerJobCoordinator::ServiceWorkerJobCoordinator(
    base::WeakPtr<ServiceWorkerContextCore> context)
    : context_(context) {
}

ServiceWorkerJobCoordinator::~ServiceWorkerJobCoordinator() {
  if (!context_) {
    for (auto& job_pair : job_queues_)
      job_pair.second.ClearForShutdown();
    job_queues_.clear();
  }
  DCHECK(job_queues_.empty()) << "Destroying ServiceWorkerJobCoordinator with "
                              << job_queues_.size() << " job queues";
}

void ServiceWorkerJobCoordinator::Register(
    const GURL& pattern,
    const GURL& script_url,
    ServiceWorkerProviderHost* provider_host,
    const ServiceWorkerRegisterJob::RegistrationCallback& callback) {
  std::unique_ptr<ServiceWorkerRegisterJobBase> job(
      new ServiceWorkerRegisterJob(context_, pattern, script_url));
  ServiceWorkerRegisterJob* queued_job = static_cast<ServiceWorkerRegisterJob*>(
      job_queues_[pattern].Push(std::move(job)));
  queued_job->AddCallback(callback, provider_host);
}

void ServiceWorkerJobCoordinator::Unregister(
    const GURL& pattern,
    const ServiceWorkerUnregisterJob::UnregistrationCallback& callback) {
  std::unique_ptr<ServiceWorkerRegisterJobBase> job(
      new ServiceWorkerUnregisterJob(context_, pattern));
  ServiceWorkerUnregisterJob* queued_job =
      static_cast<ServiceWorkerUnregisterJob*>(
          job_queues_[pattern].Push(std::move(job)));
  queued_job->AddCallback(callback);
}

void ServiceWorkerJobCoordinator::Update(
    ServiceWorkerRegistration* registration,
    bool force_bypass_cache) {
  DCHECK(registration);
  job_queues_[registration->pattern()].Push(
      base::WrapUnique<ServiceWorkerRegisterJobBase>(
          new ServiceWorkerRegisterJob(context_, registration,
                                       force_bypass_cache,
                                       false /* skip_script_comparison */)));
}

void ServiceWorkerJobCoordinator::Update(
    ServiceWorkerRegistration* registration,
    bool force_bypass_cache,
    bool skip_script_comparison,
    ServiceWorkerProviderHost* provider_host,
    const ServiceWorkerRegisterJob::RegistrationCallback& callback) {
  DCHECK(registration);
  ServiceWorkerRegisterJob* queued_job = static_cast<ServiceWorkerRegisterJob*>(
      job_queues_[registration->pattern()].Push(
          base::WrapUnique<ServiceWorkerRegisterJobBase>(
              new ServiceWorkerRegisterJob(context_, registration,
                                           force_bypass_cache,
                                           skip_script_comparison))));
  queued_job->AddCallback(callback, provider_host);
}

void ServiceWorkerJobCoordinator::AbortAll() {
  for (auto& job_pair : job_queues_)
    job_pair.second.AbortAll();
  job_queues_.clear();
}

void ServiceWorkerJobCoordinator::FinishJob(const GURL& pattern,
                                            ServiceWorkerRegisterJobBase* job) {
  auto pending_jobs = job_queues_.find(pattern);
  DCHECK(pending_jobs != job_queues_.end()) << "Deleting non-existent job.";
  pending_jobs->second.Pop(job);
  if (pending_jobs->second.empty())
    job_queues_.erase(pending_jobs);
}

}  // namespace content
