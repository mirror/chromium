// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_DIAL_DIAL_APP_DISCOVERY_SERVICE_H_
#define CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_DIAL_DIAL_APP_DISCOVERY_SERVICE_H_

#include <memory>
#include <set>
#include <string>

#include "base/callback.h"
#include "base/containers/flat_map.h"
#include "base/gtest_prod_util.h"
#include "base/sequence_checker.h"
#include "chrome/browser/media/router/discovery/dial/parsed_dial_app_info.h"
#include "chrome/browser/media/router/discovery/dial/safe_dial_app_info_parser.h"
#include "chrome/common/media_router/discovery/media_sink_internal.h"
#include "url/gurl.h"

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
enum class SinkAppStatus { kAvailable, kUnavailable, kUnknown };

// This class provides an API to fetch DIAL app info XML from an app URL and
// parse the XML into a DialAppInfo object. Actual parsing happens in a
// separate utility process via SafeDialAppInfoParser instead of in this class.
// During shutdown, this class abort all pending requests and no callbacks get
// invoked.
// This class lives on the IO thread.
class DialAppDiscoveryService {
 public:
  // Called if parsing app info XML in utility process succeeds, and all fields
  // are valid.
  // |sink|: Media sink object initiating the HTTP GET request.
  // |app_name|: app name passed in when initiating the HTTP GET request.
  // |app_status|: app status indicating if |app_name| is available on |sink|.
  using DialAppInfoParseSuccessCallback =
      base::RepeatingCallback<void(const MediaSinkInternal& sink,
                                   const std::string& app_name,
                                   SinkAppStatus app_status)>;

  // Called if parsing app info XML in utility process fails, or some parsed
  // fields are missing or invalid.
  // |sink|: Media sink object initiating the HTTP GET request.
  // |app_name|: app name passed in when initiating the HTTP GET request.
  // |error_message|: error message indicating why app info XML parsing fails.
  using DialAppInfoParseErrorCallback =
      base::RepeatingCallback<void(const MediaSinkInternal& sink,
                                   const std::string& app_name,
                                   const std::string& error_message)>;

  DialAppDiscoveryService(service_manager::Connector* connector,
                          const DialAppInfoParseSuccessCallback& success_cb,
                          const DialAppInfoParseErrorCallback& error_cb);

  virtual ~DialAppDiscoveryService();

  // Queries |app_name|'s availability on |sink| by issues a HTTP GET request.
  // No-op if there is already a pending request.
  // App URL is used to issue HTTP GET request. E.g.
  // http://127.0.0.1/apps/YouTube. "http://127.0.0.1/apps/" is the base part
  // which comes from |sink|; "YouTube" suffix is the app name part which comes
  // from |app_name|.
  // |request_context|: Used by the background URLFetchers.
  virtual void FetchDialAppInfo(const MediaSinkInternal& sink,
                                const std::string& app_name,
                                net::URLRequestContextGetter* request_context);

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

  // Used by unit test.
  void SetParserForTest(std::unique_ptr<SafeDialAppInfoParser> parser);

  // Invoked when HTTP GET request finishes.
  // |sink|: Media sink object initiating the HTTP GET request.
  // |app_name|: app name passed in when initiating the HTTP GET request.
  // |app_info_xml|: Response XML from HTTP request.
  void OnDialAppInfoFetchComplete(const MediaSinkInternal& sink,
                                  const std::string& app_name,
                                  const std::string& app_info_xml);

  // Invoked when HTTP GET request fails.
  // |sink|: Media sink object initiating the HTTP GET request.
  // |app_name|: app name passed in when initiating the HTTP GET request.
  // |response_code|: The HTTP response code received.
  // |error_message|: Error message from HTTP request.
  void OnDialAppInfoFetchError(const MediaSinkInternal& sink,
                               const std::string& app_name,
                               int response_code,
                               const std::string& error_message);

  // Invoked when SafeDialAppInfoParser finishes parsing app info XML.
  // |sink|: Media sink object initiating the HTTP GET request.
  // |app_name|: app name passed in when initiating the HTTP GET request.
  // |app_info|: Parsed app info from utility process, or nullptr if parsing
  // failed.
  // |mojo_parsing_error|: error encountered while parsing DIAL app info XML.
  void OnDialAppInfo(const MediaSinkInternal& sink,
                     const std::string& app_name,
                     std::unique_ptr<ParsedDialAppInfo> app_info,
                     SafeDialAppInfoParser::ParsingError parsing_error);

  // Map of pending app info fetchers, keyed by request id.
  base::flat_map<std::string, std::unique_ptr<DialAppInfoFetcher>>
      pending_fetcher_map_;

  // See comments for DialAppInfoParseSuccessCallback.
  DialAppInfoParseSuccessCallback success_cb_;

  // See comments for DialAppInfoParseErrorCallback.
  DialAppInfoParseErrorCallback error_cb_;

  // Safe DIAL parser. Does the parsing in a utility process.
  std::unique_ptr<SafeDialAppInfoParser> parser_;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_DIAL_DIAL_APP_DISCOVERY_SERVICE_H_
