// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_DIAL_DIAL_APP_DISCOVERY_SERVICE_H_
#define CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_DIAL_DIAL_APP_DISCOVERY_SERVICE_H_

#include <memory>
#include <set>
#include <string>

#include "base/callback.h"
#include "base/threading/thread_checker.h"
#include "chrome/common/media_router/mojo/dial_app_info_parser.mojom.h"
#include "url/gurl.h"

namespace net {
class URLRequestContextGetter;
}

namespace media_router {

class DialAppInfoFetcher;
class SafeDialAppInfoParser;

// Represents DIAL app status on receiver device.
enum class DialAppStatus { AVAILABLE, UNAVAILABLE, UNKNOWN };

// Represents a DIAl app info object parsed from app info XML.
struct ParsedDialAppInfo {
  // Name of the app, e.g. YouTube, Netflix.
  std::string app_name;

  // Status of the app.
  DialAppStatus app_status;
};

// This class fetches and parses app info XML for DIAL devices. Actual
// parsing happens in a separate utility process via SafeDialAppInfoParser
// (instead of in this class). This class lives on IO thread.
class DialAppDiscoveryService {
 public:
  // Represents cached app info data parsed from app info XML.
  struct CacheEntry {
    CacheEntry();
    CacheEntry(const CacheEntry& other);
    ~CacheEntry();

    // The expiration time from the cache.
    base::Time expire_time;

    // Parsed app info data from XML.
    ParsedDialAppInfo app_info;
  };

  // Called if parsing app info XML in utility process succeeds, and all fields
  // are valid.
  // |app_url|: URL to issue HTTP GET request and get app info XML.
  // |app_info|: Parsed app info object from app info XML.
  using DialAppInfoParseSuccessCallback =
      base::Callback<void(const GURL& app_url,
                          const ParsedDialAppInfo& app_info)>;

  // Called if parsing app info XML in utility process fails, or some parsed
  // fields are missing or invalid.
  // |app_url|: URL to issue HTTP GET request and get app info XML.
  // |error_message|: error message indicating why app info XML parsing fails.
  using DialAppInfoParseErrorCallback =
      base::Callback<void(const GURL& app_url,
                          const std::string& error_message)>;

  DialAppDiscoveryService(const DialAppInfoParseSuccessCallback& success_cb,
                          const DialAppInfoParseErrorCallback& error_cb);
  virtual ~DialAppDiscoveryService();

  // Issues a HTTP GET request for the app info. No-op if there is already a
  // pending request.
  // |partial_app_url|: combines with |app_name| to form an app URL, used to
  // issue HTTP GET request and get app info XML.
  // |request_context|: Used by the background URLFetchers.
  void FetchDialAppInfo(const GURL& partial_app_url,
                        const std::string& app_name,
                        net::URLRequestContextGetter* request_context);

 private:
  // Checks the cache for a valid app info. If the entry is found but is
  // expired, it is removed from the cache. Returns cached entry of parsed
  // app info. Returns nullptr if cache entry does not exist or is not valid.
  // |app_url|: URL to issue HTTP GET request and get app info XML.
  const CacheEntry* CheckAndUpdateCache(const GURL& app_url);

  // Invoked when HTTP GET request finishes.
  // |app_url|: URL to issue HTTP GET request and get app info XML.
  // |app_info_xml|: Response XML from HTTP request.
  void OnDialAppInfoFetchComplete(const GURL& app_url,
                                  const std::string& app_info_xml);

  // Invoked when HTTP GET request fails.
  // |app_url|: URL to issue HTTP GET request and get app info XML.
  // |error_message|: Error message from HTTP request.
  void OnDialAppInfoFetchError(const GURL& app_url,
                               const std::string& error_message);

  // Invoked when SafeDialAppInfoParser finishes parsing app info XML.
  // |app_url|: URL to issue HTTP GET request and get app info XML.
  // |mojo_app_info|: Parsed app info from utility process, or nullptr if
  // parsing failed.
  // |mojo_parsing_error|: error encountered while parsing DIAL app info XML.
  void OnParsedDialAppInfo(
      const GURL& app_url,
      chrome::mojom::DialAppInfoPtr mojo_app_info,
      chrome::mojom::DialAppInfoParsingError mojo_parsing_error);

  // Used by unit tests.
  virtual base::Time GetNow();

  // Map of current app info fetches in progress, keyed by app url.
  std::map<GURL, std::unique_ptr<DialAppInfoFetcher>> app_info_fetcher_map_;

  // Set of app url to be parsed in current utility process.
  std::set<GURL> pending_app_urls_;

  // Map of current cached app info, keyed by app url.
  std::map<GURL, CacheEntry> app_info_cache_;

  // See comments for DialAppInfoParseSuccessCallback.
  DialAppInfoParseSuccessCallback success_cb_;

  // See comments for DialAppInfoParseErrorCallback.
  DialAppInfoParseErrorCallback error_cb_;

  // Safe DIAL parser associated with utility process. When this object is
  // destroyed, associating utility process will shutdown. Keep |parser_| alive
  // until finish parsing all app info passed in from |FetchDialAppInfo()|. If
  // second |FetchDialAppInfo()| arrives and |parser_| is still alive, reuse
  // |parser_| instead of creating a new object.
  std::unique_ptr<SafeDialAppInfoParser> parser_;

  THREAD_CHECKER(thread_checker_);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_DIAL_DIAL_APP_DISCOVERY_SERVICE_H_
