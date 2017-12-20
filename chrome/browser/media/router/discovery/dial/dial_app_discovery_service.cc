// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/discovery/dial/dial_app_discovery_service.h"

#include "base/memory/ptr_util.h"
#include "base/stl_util.h"
#include "base/strings/string_util.h"
#include "base/time/default_clock.h"
#include "chrome/browser/media/router/discovery/dial/dial_app_info_fetcher.h"
#include "chrome/browser/media/router/discovery/dial/safe_dial_app_info_parser.h"
#include "net/http/http_status_code.h"
#include "url/gurl.h"

namespace {

// Maximum size on the number of cached entries.
constexpr int kCacheMaxEntries = 256;

media_router::DialAppStatus GetAppStatusFromAppInfo(
    const media_router::ParsedDialAppInfo& app_info) {
  if (app_info.state == media_router::DialAppState::kRunning ||
      app_info.state == media_router::DialAppState::kStopped) {
    return media_router::DialAppStatus::kAvailable;
  }

  return media_router::DialAppStatus::kUnavailable;
}

}  // namespace

namespace media_router {

DialAppDiscoveryService::FetcherStatus::FetcherStatus() = default;
DialAppDiscoveryService::FetcherStatus::~FetcherStatus() = default;

DialAppDiscoveryService::DialAppDiscoveryService(
    service_manager::Connector* connector,
    const DialAppInfoParseSuccessCallback& success_cb,
    const DialAppInfoParseErrorCallback& error_cb)
    : success_cb_(success_cb),
      error_cb_(error_cb),
      clock_(new base::DefaultClock()) {
  parser_ = base::MakeUnique<SafeDialAppInfoParser>(connector);
}

DialAppDiscoveryService::~DialAppDiscoveryService() = default;

// Background requests should respect the cache but foreground requests should
// always query the device to get the current state (and update the cache)
void DialAppDiscoveryService::FetchDialAppInfo(
    const GURL& app_url,
    net::URLRequestContextGetter* request_context,
    bool force_fetch) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DVLOG(2) << "Fetch DIAL app info from: " << app_url.spec();

  if (!force_fetch) {
    auto cache_it = app_info_cache_.find(app_url);
    if (cache_it != app_info_cache_.end()) {
      success_cb_.Run(app_url, cache_it->second);
      return;
    }
  }

  FetcherStatus* fetcher_status = nullptr;
  auto fetcher_it = app_info_fetcher_map_.find(app_url);
  if (fetcher_it == app_info_fetcher_map_.end()) {
    fetcher_status = new FetcherStatus();
    app_info_fetcher_map_.insert(std::make_pair(
        app_url, std::unique_ptr<FetcherStatus>(fetcher_status)));
  } else {
    fetcher_status = fetcher_it->second.get();
  }

  if (fetcher_status->app_info_fetcher ||
      fetcher_status->last_error == FetcherStatus::FetchError::kParsePending) {
    DVLOG(2) << "There is a pending fetch request for: " << app_url.spec();
    return;
  }

  if (fetcher_status->last_error ==
          FetcherStatus::FetchError::kInvalidResponse ||
      fetcher_status->last_error == FetcherStatus::FetchError::kParseError) {
    error_cb_.Run(app_url,
                  "Last fetch failed due to parsing error or unavailable "
                  "HTTP response.");
    return;
  }

  fetcher_status->app_info_fetcher = base::WrapUnique(new DialAppInfoFetcher(
      app_url, request_context,
      base::BindOnce(&DialAppDiscoveryService::OnDialAppInfoFetchComplete,
                     base::Unretained(this), app_url, fetcher_status),
      base::BindOnce(&DialAppDiscoveryService::OnDialAppInfoFetchError,
                     base::Unretained(this), app_url, fetcher_status)));

  fetcher_status->app_info_fetcher->Start();
}

void DialAppDiscoveryService::SetClockForTest(
    std::unique_ptr<base::Clock> clock) {
  clock_ = std::move(clock);
}

void DialAppDiscoveryService::SetParserForTest(
    std::unique_ptr<SafeDialAppInfoParser> parser) {
  parser_ = std::move(parser);
}

void DialAppDiscoveryService::OnDialAppInfo(
    const GURL& app_url,
    FetcherStatus* out_fetcher_status,
    std::unique_ptr<ParsedDialAppInfo> parsed_app_info,
    SafeDialAppInfoParser::ParsingError parsing_error) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(out_fetcher_status);

  if (!parsed_app_info) {
    DVLOG(2) << "Failed to parse app info XML in utility process, error: "
             << static_cast<int>(parsing_error);
    out_fetcher_status->last_error = FetcherStatus::FetchError::kParseError;
    error_cb_.Run(app_url, "Failed to parse app info XML in utility process");
    return;
  }

  DialAppInfo app_info;
  app_info.app_name = parsed_app_info->name;
  app_info.app_status = GetAppStatusFromAppInfo(*parsed_app_info);

  DVLOG(2) << "Get parsed DIAL app info from utility process, [app_url]: "
           << app_url.spec() << " [name]: " << app_info.app_name
           << " [status]: " << static_cast<int>(app_info.app_status);

  if (app_info_cache_.size() < kCacheMaxEntries) {
    DVLOG(2) << "Caching app info for " << app_url.spec();
    app_info_cache_[app_url] = app_info;
  }

  out_fetcher_status->last_error = FetcherStatus::FetchError::kNone;
  success_cb_.Run(app_url, app_info);
}

void DialAppDiscoveryService::OnDialAppInfoFetchComplete(
    const GURL& app_url,
    FetcherStatus* out_fetcher_status,
    const std::string& app_info_xml) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(out_fetcher_status);

  out_fetcher_status->last_error = FetcherStatus::FetchError::kParsePending;
  out_fetcher_status->app_info_fetcher.reset();

  parser_->Parse(
      app_info_xml,
      base::BindOnce(&DialAppDiscoveryService::OnDialAppInfo,
                     base::Unretained(this), app_url, out_fetcher_status));
}

void DialAppDiscoveryService::OnDialAppInfoFetchError(
    const GURL& app_url,
    FetcherStatus* out_fetcher_status,
    int response_code,
    const std::string& error_message) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(out_fetcher_status);
  DVLOG(2) << "Fail to fetch app info XML from: " << app_url.spec()
           << " due to error: " << error_message;

  FetcherStatus::FetchError error = FetcherStatus::FetchError::kTransientError;
  if (response_code == net::HTTP_NOT_FOUND ||
      response_code >= net::HTTP_INTERNAL_SERVER_ERROR) {
    error = FetcherStatus::FetchError::kUnavailableResponse;
  } else if (response_code == net::HTTP_OK) {
    error = FetcherStatus::FetchError::kInvalidResponse;
  }

  out_fetcher_status->last_error = error;
  out_fetcher_status->app_info_fetcher.reset();

  if (error != FetcherStatus::FetchError::kTransientError)
    error_cb_.Run(app_url, error_message);
}

}  // namespace media_router
