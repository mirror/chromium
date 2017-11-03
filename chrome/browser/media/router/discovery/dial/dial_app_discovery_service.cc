// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/discovery/dial/dial_app_discovery_service.h"

#include "base/stl_util.h"
#include "base/strings/string_util.h"
#include "chrome/browser/media/router/discovery/dial/dial_app_info_fetcher.h"
#include "chrome/browser/media/router/discovery/dial/safe_dial_app_info_parser.h"
#include "url/gurl.h"

namespace {

// How long to cache an app info.
constexpr int kDialAppInfoCacheTimeHours = 1;

// Maximum size on the number of cached entries.
constexpr int kCacheMaxEntries = 256;

media_router::DialAppStatus GetAvailabilityFromAppInfo(
    chrome::mojom::DialAppInfoPtr mojo_app_info) {
  media_router::DialAppStatus app_status =
      media_router::DialAppStatus::UNAVAILABLE;
  if (mojo_app_info->state == chrome::mojom::DialAppState::RUNNING ||
      mojo_app_info->state == chrome::mojom::DialAppState::STOPPED) {
    app_status = media_router::DialAppStatus::AVAILABLE;
  }

  // Compuate Netfix app status.
  if (mojo_app_info->name == "Netflix") {
    if (!mojo_app_info->capabilities.has_value() ||
        mojo_app_info->capabilities.value() != "websocket") {
      app_status = media_router::DialAppStatus::UNAVAILABLE;
    }
  }

  return app_status;
}

GURL CreateAppURL(const GURL& partial_app_url, const std::string& app_name) {
  return GURL(partial_app_url.spec() + app_name);
}

}  // namespace

namespace media_router {

DialAppDiscoveryService::DialAppDiscoveryService(
    const DialAppInfoParseSuccessCallback& success_cb,
    const DialAppInfoParseErrorCallback& error_cb)
    : success_cb_(success_cb), error_cb_(error_cb) {}

DialAppDiscoveryService::~DialAppDiscoveryService() {
  if (!pending_app_urls_.empty()) {
    DLOG(WARNING) << "Fail to finish parsing " << pending_app_urls_.size()
                  << " app info.";
  }
}

void DialAppDiscoveryService::FetchDialAppInfo(
    const GURL& partial_app_url,
    const std::string& app_name,
    net::URLRequestContextGetter* request_context) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  GURL app_url = CreateAppURL(partial_app_url, app_name);
  DVLOG(2) << "Fetch DIAL app info from: " << app_url.spec();

  auto* cache_entry = CheckAndUpdateCache(app_url);
  // Get app info from cache.
  if (cache_entry) {
    success_cb_.Run(app_url, cache_entry->app_info);
    return;
  }

  // No op if there is an existing Fetcher.
  if (!pending_app_urls_.insert(app_url).second) {
    DVLOG(2) << "There is a pending fetch request for: " << app_url.spec();
    return;
  }

  auto app_info_fetcher = base::WrapUnique(new DialAppInfoFetcher(
      app_url, request_context,
      base::BindOnce(&DialAppDiscoveryService::OnDialAppInfoFetchComplete,
                     base::Unretained(this), app_url),
      base::BindOnce(&DialAppDiscoveryService::OnDialAppInfoFetchError,
                     base::Unretained(this), app_url)));

  app_info_fetcher->Start();
  app_info_fetcher_map_.insert(
      std::make_pair(app_url, std::move(app_info_fetcher)));
}

const DialAppDiscoveryService::CacheEntry*
DialAppDiscoveryService::CheckAndUpdateCache(const GURL& app_url) {
  const auto& it = app_info_cache_.find(app_url);
  if (it == app_info_cache_.end())
    return nullptr;

  // If the entry's config_id does not match, or it has expired, remove it.
  if (GetNow() >= it->second.expire_time) {
    DVLOG(2) << "Removing invalid entry " << it->first;
    app_info_cache_.erase(it);
    return nullptr;
  }

  // Entry is valid.
  return &it->second;
}

void DialAppDiscoveryService::OnParsedDialAppInfo(
    const GURL& app_url,
    chrome::mojom::DialAppInfoPtr mojo_app_info,
    chrome::mojom::DialAppInfoParsingError mojo_parsing_error) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  pending_app_urls_.erase(app_url);
  // Last callback for current utility process. Release |parser_| and
  // SafeDialDialAppInfoParser object will be destroyed after this
  // callback.
  if (pending_app_urls_.empty())
    parser_.reset();

  if (!mojo_app_info) {
    error_cb_.Run(app_url, "Failed to parse app info XML in utility process");
    return;
  }

  ParsedDialAppInfo app_info;
  app_info.app_name = mojo_app_info->name;
  app_info.app_status = GetAvailabilityFromAppInfo(std::move(mojo_app_info));

  DVLOG(2) << "Get parsed DIAL app info from utility process, [app_url]: "
           << app_url.spec() << " [name]: " << app_info.app_name
           << " [status]: " << static_cast<int>(app_info.app_status);

  if (app_info_cache_.size() >= kCacheMaxEntries) {
    success_cb_.Run(app_url, app_info);
    return;
  }

  DVLOG(2) << "Caching app info for " << app_url.spec();
  CacheEntry cached_app_info;
  cached_app_info.expire_time =
      GetNow() + base::TimeDelta::FromHours(kDialAppInfoCacheTimeHours);
  cached_app_info.app_info = app_info;
  app_info_cache_.insert(std::make_pair(app_url, cached_app_info));

  success_cb_.Run(app_url, app_info);
}

void DialAppDiscoveryService::OnDialAppInfoFetchComplete(
    const GURL& app_url,
    const std::string& app_info_xml) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  if (!parser_)
    parser_ = base::MakeUnique<SafeDialAppInfoParser>();

  parser_->Start(app_info_xml,
                 base::Bind(&DialAppDiscoveryService::OnParsedDialAppInfo,
                            base::Unretained(this), app_url));

  app_info_fetcher_map_.erase(app_url);
}

void DialAppDiscoveryService::OnDialAppInfoFetchError(
    const GURL& app_url,
    const std::string& error_message) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DVLOG(2) << "Fail to fetch app info XML from: " << app_url.spec()
           << " due to error: " << error_message;

  error_cb_.Run(app_url, error_message);
  app_info_fetcher_map_.erase(app_url);
}

base::Time DialAppDiscoveryService::GetNow() {
  return base::Time::Now();
}

DialAppDiscoveryService::CacheEntry::CacheEntry() {}
DialAppDiscoveryService::CacheEntry::CacheEntry(const CacheEntry& other) =
    default;
DialAppDiscoveryService::CacheEntry::~CacheEntry() = default;

}  // namespace media_router
