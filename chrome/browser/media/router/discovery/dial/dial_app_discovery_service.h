// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_DIAL_DIAL_APP_DISCOVERY_SERVICE_H_
#define CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_DIAL_DIAL_APP_DISCOVERY_SERVICE_H_

#include <memory>
#include <set>
#include <string>

#include "base/callback.h"
#include "base/gtest_prod_util.h"
#include "base/threading/thread_checker.h"
#include "chrome/browser/media/router/discovery/dial/parsed_dial_app_info.h"
#include "chrome/browser/media/router/discovery/dial/safe_dial_app_info_parser.h"
#include "url/gurl.h"

namespace base {
class Clock;
}

namespace net {
class URLRequestContextGetter;
}

namespace service_manager {
class Connector;
}

namespace media_router {

class DialAppInfoFetcher;
class SafeDialAppInfoParser;

// Represents DIAL app status on receiver device.
enum class DialAppStatus { kAvailable, kUnavailable, kUnknown };

// Represents a DIAl app info object parsed from app info XML.
struct DialAppInfo {
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
    DialAppInfo app_info;
  };

  // Called if parsing app info XML in utility process succeeds, and all fields
  // are valid.
  // |app_url|: URL to issue HTTP GET request and get app info XML.
  // |app_info|: Parsed app info object from app info XML.
  using DialAppInfoParseSuccessCallback =
      base::Callback<void(const GURL& app_url, const DialAppInfo& app_info)>;

  // Called if parsing app info XML in utility process fails, or some parsed
  // fields are missing or invalid.
  // |app_url|: URL to issue HTTP GET request and get app info XML.
  // |error_message|: error message indicating why app info XML parsing fails.
  using DialAppInfoParseErrorCallback =
      base::Callback<void(const GURL& app_url,
                          const std::string& error_message)>;

  DialAppDiscoveryService(service_manager::Connector* connector,
                          const DialAppInfoParseSuccessCallback& success_cb,
                          const DialAppInfoParseErrorCallback& error_cb);
  virtual ~DialAppDiscoveryService();

  // Issues a HTTP GET request for the app info. No-op if there is already a
  // pending request.
  // |app_url|:  app URL, used to issue HTTP GET request and get app info XML.
  // |request_context|: Used by the background URLFetchers.
  void FetchDialAppInfo(const GURL& app_url,
                        net::URLRequestContextGetter* request_context);

  // Used by unit test.
  void SetClockForTest(std::unique_ptr<base::Clock> clock);
  void SetParserForTest(std::unique_ptr<SafeDialAppInfoParser> parser);

 private:
  friend class DialAppDiscoveryServiceTest;
  FRIEND_TEST_ALL_PREFIXES(DialAppDiscoveryServiceTest,
                           TestFechDialAppInfoFromCache);
  FRIEND_TEST_ALL_PREFIXES(DialAppDiscoveryServiceTest,
                           TestFechDialAppInfoFromCacheExpiredEntry);
  FRIEND_TEST_ALL_PREFIXES(DialAppDiscoveryServiceTest,
                           TestSafeParserProperlyCreated);
  FRIEND_TEST_ALL_PREFIXES(DialAppDiscoveryServiceTest,
                           TestGetAvailabilityFromAppInfoAvailable);
  FRIEND_TEST_ALL_PREFIXES(DialAppDiscoveryServiceTest,
                           TestGetAvailabilityFromAppInfoUnavailable);

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
  // |app_info|: Parsed app info from utility process, or nullptr if parsing
  // failed.
  // |mojo_parsing_error|: error encountered while parsing DIAL app info XML.
  void OnDialAppInfo(const GURL& app_url,
                     std::unique_ptr<ParsedDialAppInfo> app_info,
                     SafeDialAppInfoParser::ParsingError parsing_error);

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

  // Safe DIAL parser. Does the parsing in a utility process.
  std::unique_ptr<SafeDialAppInfoParser> parser_;

  std::unique_ptr<base::Clock> clock_;

  THREAD_CHECKER(thread_checker_);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_DIAL_DIAL_APP_DISCOVERY_SERVICE_H_
