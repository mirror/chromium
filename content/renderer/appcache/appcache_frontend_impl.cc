// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/appcache/appcache_frontend_impl.h"

#include "base/logging.h"
#include "content/renderer/appcache/web_application_cache_host_impl.h"
#include "third_party/WebKit/public/web/WebConsoleMessage.h"

using blink::WebApplicationCacheHost;
using blink::WebConsoleMessage;

namespace content {

// Inline helper to keep the lines shorter and unwrapped.
inline WebApplicationCacheHostImpl* GetHost(int id) {
  return WebApplicationCacheHostImpl::FromId(id);
}

void AppCacheFrontendImpl::OnCacheSelected(int host_id,
                                           const AppCacheInfo& info) {
  WebApplicationCacheHostImpl* host = GetHost(host_id);
  if (host)
    host->OnCacheSelected(info);
}

void AppCacheFrontendImpl::OnStatusChanged(const std::vector<int>& host_ids,
                                           AppCacheStatus status) {
  for (std::vector<int>::const_iterator i = host_ids.begin();
       i != host_ids.end(); ++i) {
    WebApplicationCacheHostImpl* host = GetHost(*i);
    if (host)
      host->OnStatusChanged(status);
  }
}

void AppCacheFrontendImpl::OnEventRaised(const std::vector<int>& host_ids,
                                         AppCacheEventID event_id) {
  DCHECK_NE(
      event_id,
      AppCacheEventID::APPCACHE_PROGRESS_EVENT);  // See OnProgressEventRaised.
  DCHECK_NE(event_id,
            AppCacheEventID::APPCACHE_ERROR_EVENT);  // See OnErrorEventRaised.
  for (std::vector<int>::const_iterator i = host_ids.begin();
       i != host_ids.end(); ++i) {
    WebApplicationCacheHostImpl* host = GetHost(*i);
    if (host)
      host->OnEventRaised(event_id);
  }
}

void AppCacheFrontendImpl::OnProgressEventRaised(
    const std::vector<int>& host_ids,
    const GURL& url,
    int num_total,
    int num_complete) {
  for (std::vector<int>::const_iterator i = host_ids.begin();
       i != host_ids.end(); ++i) {
    WebApplicationCacheHostImpl* host = GetHost(*i);
    if (host)
      host->OnProgressEventRaised(url, num_total, num_complete);
  }
}

void AppCacheFrontendImpl::OnErrorEventRaised(
    const std::vector<int>& host_ids,
    const AppCacheErrorDetails& details) {
  for (std::vector<int>::const_iterator i = host_ids.begin();
       i != host_ids.end(); ++i) {
    WebApplicationCacheHostImpl* host = GetHost(*i);
    if (host)
      host->OnErrorEventRaised(details);
  }
}

void AppCacheFrontendImpl::OnLogMessage(int host_id,
                                        AppCacheLogLevel log_level,
                                        const std::string& message) {
  WebApplicationCacheHostImpl* host = GetHost(host_id);
  if (host)
    host->OnLogMessage(log_level, message);
}

void AppCacheFrontendImpl::OnContentBlocked(int host_id,
                                            const GURL& manifest_url) {
  WebApplicationCacheHostImpl* host = GetHost(host_id);
  if (host)
    host->OnContentBlocked(manifest_url);
}

void AppCacheFrontendImpl::OnSetSubresourceFactory(
    int host_id,
    mojo::MessagePipeHandle loader_factory_pipe_handle) {
  WebApplicationCacheHostImpl* host = GetHost(host_id);
  if (host)
    host->SetSubresourceFactory(loader_factory_pipe_handle);
}

// Ensure that enum values never get out of sync with the
// ones declared for use within the WebKit api

#define STATUS_ASSERT_EQUAL(a, b)                            \
  static_assert(static_cast<int>(a) == static_cast<int>(b), \
                "mismatched enum: " #a)

STATUS_ASSERT_EQUAL(WebApplicationCacheHost::kUncached,
                   AppCacheStatus::APPCACHE_STATUS_UNCACHED);
STATUS_ASSERT_EQUAL(WebApplicationCacheHost::kIdle,
                   AppCacheStatus::APPCACHE_STATUS_IDLE);
STATUS_ASSERT_EQUAL(WebApplicationCacheHost::kChecking,
                   AppCacheStatus::APPCACHE_STATUS_CHECKING);
STATUS_ASSERT_EQUAL(WebApplicationCacheHost::kDownloading,
                   AppCacheStatus::APPCACHE_STATUS_DOWNLOADING);
STATUS_ASSERT_EQUAL(WebApplicationCacheHost::kUpdateReady,
                   AppCacheStatus::APPCACHE_STATUS_UPDATE_READY);
STATUS_ASSERT_EQUAL(WebApplicationCacheHost::kObsolete,
                   AppCacheStatus::APPCACHE_STATUS_OBSOLETE);

STATUS_ASSERT_EQUAL(WebApplicationCacheHost::kCheckingEvent,
                   AppCacheEventID::APPCACHE_CHECKING_EVENT);
STATUS_ASSERT_EQUAL(WebApplicationCacheHost::kErrorEvent,
                   AppCacheEventID::APPCACHE_ERROR_EVENT);
STATUS_ASSERT_EQUAL(WebApplicationCacheHost::kNoUpdateEvent,
                   AppCacheEventID::APPCACHE_NO_UPDATE_EVENT);
STATUS_ASSERT_EQUAL(WebApplicationCacheHost::kDownloadingEvent,
                   AppCacheEventID::APPCACHE_DOWNLOADING_EVENT);
STATUS_ASSERT_EQUAL(WebApplicationCacheHost::kProgressEvent,
                   AppCacheEventID::APPCACHE_PROGRESS_EVENT);
STATUS_ASSERT_EQUAL(WebApplicationCacheHost::kUpdateReadyEvent,
                   AppCacheEventID::APPCACHE_UPDATE_READY_EVENT);
STATUS_ASSERT_EQUAL(WebApplicationCacheHost::kCachedEvent,
                   AppCacheEventID::APPCACHE_CACHED_EVENT);
STATUS_ASSERT_EQUAL(WebApplicationCacheHost::kObsoleteEvent,
                   AppCacheEventID::APPCACHE_OBSOLETE_EVENT);

STATUS_ASSERT_EQUAL(WebApplicationCacheHost::kManifestError,
                   AppCacheErrorReason::APPCACHE_MANIFEST_ERROR);
STATUS_ASSERT_EQUAL(WebApplicationCacheHost::kSignatureError,
                   AppCacheErrorReason::APPCACHE_SIGNATURE_ERROR);
STATUS_ASSERT_EQUAL(WebApplicationCacheHost::kResourceError,
                   AppCacheErrorReason::APPCACHE_RESOURCE_ERROR);
STATUS_ASSERT_EQUAL(WebApplicationCacheHost::kChangedError,
                   AppCacheErrorReason::APPCACHE_CHANGED_ERROR);
STATUS_ASSERT_EQUAL(WebApplicationCacheHost::kAbortError,
                   AppCacheErrorReason::APPCACHE_ABORT_ERROR);
STATUS_ASSERT_EQUAL(WebApplicationCacheHost::kQuotaError,
                   AppCacheErrorReason::APPCACHE_QUOTA_ERROR);
STATUS_ASSERT_EQUAL(WebApplicationCacheHost::kPolicyError,
                   AppCacheErrorReason::APPCACHE_POLICY_ERROR);
STATUS_ASSERT_EQUAL(WebApplicationCacheHost::kUnknownError,
                   AppCacheErrorReason::APPCACHE_UNKNOWN_ERROR);

STATUS_ASSERT_EQUAL(WebConsoleMessage::kLevelVerbose, APPCACHE_LOG_VERBOSE);
STATUS_ASSERT_EQUAL(WebConsoleMessage::kLevelInfo, APPCACHE_LOG_INFO);
STATUS_ASSERT_EQUAL(WebConsoleMessage::kLevelWarning, APPCACHE_LOG_WARNING);
STATUS_ASSERT_EQUAL(WebConsoleMessage::kLevelError, APPCACHE_LOG_ERROR);

}  // namespace content
