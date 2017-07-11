// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/predictors/preconnect_manager.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/resource_hints.h"
#include "net/base/net_errors.h"

namespace predictors {

const size_t kMaxInflightPreresolves = 3;
const int kAllowCredentialsOnPreconnectByDefault = true;

namespace internal {

PreconnectImpl::~PreconnectImpl() = default;

void PreconnectImpl::PreconnectUrl(
    net::URLRequestContext* request_context,
    const GURL& url,
    const GURL& first_party_for_cookies,
    int count,
    bool allow_credentials,
    net::HttpRequestInfo::RequestMotivation motivation) const {
  content::PreconnectUrl(request_context, url, first_party_for_cookies, count,
                         allow_credentials, motivation);
}

int PreconnectImpl::PreresolveUrl(
    net::URLRequestContext* request_context,
    const GURL& url,
    const net::CompletionCallback& callback) const {
  return content::PreresolveUrl(request_context, url, callback);
}

}  // namespace internal

PreresolveInfo::PreresolveInfo(const GURL& i_url, size_t count)
    : url(i_url), queued_count(count), inflight_count(0), was_canceled(false) {}

PreresolveInfo::PreresolveInfo(const PreresolveInfo& other) = default;
PreresolveInfo::~PreresolveInfo() = default;

PreresolveJob::PreresolveJob(const GURL& i_url,
                             bool i_need_preconnect,
                             PreresolveInfo* i_info)
    : url(i_url), need_preconnect(i_need_preconnect), info(i_info) {}

PreresolveJob::PreresolveJob(const PreresolveJob& other) = default;
PreresolveJob::~PreresolveJob() = default;

PreconnectManager::PreconnectManager(
    base::WeakPtr<Delegate> delegate,
    scoped_refptr<net::URLRequestContextGetter> context_getter)
    : preconnect_impl_(base::MakeUnique<internal::PreconnectImpl>()),
      delegate_(std::move(delegate)),
      context_getter_(std::move(context_getter)),
      inflight_preresolves_count_(0),
      weak_factory_(this) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

PreconnectManager::~PreconnectManager() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
}

void PreconnectManager::Start(const GURL& url,
                              const std::vector<GURL>& preconnect_origins,
                              const std::vector<GURL>& preresolve_hosts) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  if (preresolve_info_.find(url) != preresolve_info_.end())
    return;

  auto iterator_and_whether_inserted = preresolve_info_.emplace(
      url, base::MakeUnique<PreresolveInfo>(
               url, preconnect_origins.size() + preresolve_hosts.size()));
  PreresolveInfo* info = iterator_and_whether_inserted.first->second.get();

  for (const GURL& origin : preconnect_origins)
    preresolve_queue_.emplace_back(origin, true, info);

  for (const GURL& host : preresolve_hosts)
    preresolve_queue_.emplace_back(host, false, info);

  TryToLaunchPreresolveJobs();
}

void PreconnectManager::Stop(const GURL& url) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  auto it = preresolve_info_.find(url);
  if (it == preresolve_info_.end()) {
    return;
  }

  it->second->was_canceled = true;
}

void PreconnectManager::TryToLaunchPreresolveJobs() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  while (!preresolve_queue_.empty() &&
         inflight_preresolves_count_ < kMaxInflightPreresolves) {
    auto& job = preresolve_queue_.front();
    PreresolveInfo* info = job.info;

    if (!info->was_canceled) {
      int status = preconnect_impl_->PreresolveUrl(
          context_getter_->GetURLRequestContext(), job.url,
          base::Bind(&PreconnectManager::OnPreresolveFinished,
                     weak_factory_.GetWeakPtr(), job));
      if (status == net::ERR_IO_PENDING) {
        // Will complete asynchronously.
        ++info->inflight_count;
        ++inflight_preresolves_count_;
      } else {
        // Completed synchronously (was already cached by HostResolver), or else
        // there was (equivalently) some network error that prevents us from
        // finding the name. Status net::OK means it was "found."
        FinishPreresolve(job, status == net::OK);
      }
    }

    preresolve_queue_.pop_front();
    --info->queued_count;
    if (info->is_done())
      AllPreresolvesForUrlFinished(info);
  }
}

void PreconnectManager::OnPreresolveFinished(const PreresolveJob& job,
                                             int result) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  FinishPreresolve(job, result == net::OK);
  --inflight_preresolves_count_;
  --job.info->inflight_count;
  if (job.info->is_done())
    AllPreresolvesForUrlFinished(job.info);
  TryToLaunchPreresolveJobs();
}

void PreconnectManager::FinishPreresolve(const PreresolveJob& job, bool found) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  if (found && job.need_preconnect && !job.info->was_canceled) {
    preconnect_impl_->PreconnectUrl(context_getter_->GetURLRequestContext(),
                                    job.url, job.info->url, 1,
                                    kAllowCredentialsOnPreconnectByDefault,
                                    net::HttpRequestInfo::PRECONNECT_MOTIVATED);
  }
}

void PreconnectManager::AllPreresolvesForUrlFinished(PreresolveInfo* info) {
  DCHECK(info->is_done());
  auto it = preresolve_info_.find(info->url);
  DCHECK(it != preresolve_info_.end());
  DCHECK(info == it->second.get());
  if (delegate_)
    delegate_->PreconnectFinished(info->url);
  preresolve_info_.erase(it);
}

}  // namespace predictors
