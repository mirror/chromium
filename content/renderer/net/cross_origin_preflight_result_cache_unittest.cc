// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/net/cross_origin_preflight_result_cache.h"
#include "base/memory/ptr_util.h"
#include "base/stl_util.h"
#include "content/public/test/mock_render_thread.h"
#include "net/http/http_response_headers.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/WebKit/public/platform/modules/serviceworker/WebServiceWorkerResponseType.h"

namespace content {

namespace test {
class TestCrossOriginPreflightResultCache
    : public CrossOriginPreflightResultCache {
 public:
  TestCrossOriginPreflightResultCache() {}
};
}  // namespace test

class CrossOriginPreflightResultCacheTest : public ::testing::Test {
 protected:
  std::unique_ptr<CrossOriginPreflightResultCacheItem> CreateCacheItem(
      const std::string allow_methods,
      const std::string allow_headers,
      FetchCredentialsMode credentials_mode,
      const unsigned max_age = 0) {
    std::string raw_headers = "HTTP/1.1 200 OK";
    raw_headers.push_back('\0');

    if (!allow_methods.empty()) {
      raw_headers.append("Access-Control-Allow-Methods: " + allow_methods);
      raw_headers.push_back('\0');
    }

    if (!allow_headers.empty()) {
      raw_headers.append("Access-Control-Allow-Headers: " + allow_headers);
      raw_headers.push_back('\0');
    }

    if (max_age) {
      raw_headers.append("Access-Control-Max-Age: " + std::to_string(max_age));
      raw_headers.push_back('\0');
    }

    raw_headers.push_back('\0');

    ResourceResponseHead head;
    head.headers =
        make_scoped_refptr(new net::HttpResponseHeaders(raw_headers));

    std::string error;
    return CrossOriginPreflightResultCacheItem::Create(credentials_mode, head,
                                                       error);
  }
};

TEST_F(CrossOriginPreflightResultCacheTest, CanSkipPreflight) {
  const struct {
    const char* const allow_methods;
    const char* const allow_headers;
    const FetchCredentialsMode cache_credentials_mode;

    const char* const request_method;
    const char* const request_headers;
    const FetchCredentialsMode request_credentials_mode;

    bool can_skip_preflight;
  } tests[] = {
      // Different methods:
      {"OPTIONS", "", FETCH_CREDENTIALS_MODE_OMIT, "OPTIONS", "",
       FETCH_CREDENTIALS_MODE_OMIT, true},
      {"GET", "", FETCH_CREDENTIALS_MODE_OMIT, "GET", "",
       FETCH_CREDENTIALS_MODE_OMIT, true},
      {"HEAD", "", FETCH_CREDENTIALS_MODE_OMIT, "HEAD", "",
       FETCH_CREDENTIALS_MODE_OMIT, true},
      {"POST", "", FETCH_CREDENTIALS_MODE_OMIT, "POST", "",
       FETCH_CREDENTIALS_MODE_OMIT, true},
      {"PUT", "", FETCH_CREDENTIALS_MODE_OMIT, "PUT", "",
       FETCH_CREDENTIALS_MODE_OMIT, true},
      {"DELETE", "", FETCH_CREDENTIALS_MODE_OMIT, "DELETE", "",
       FETCH_CREDENTIALS_MODE_OMIT, true},

      // Cache allowing multiple methods:
      {"GET, PUT, DELETE", "", FETCH_CREDENTIALS_MODE_OMIT, "GET", "",
       FETCH_CREDENTIALS_MODE_OMIT, true},
      {"GET, PUT, DELETE", "", FETCH_CREDENTIALS_MODE_OMIT, "PUT", "",
       FETCH_CREDENTIALS_MODE_OMIT, true},
      {"GET, PUT, DELETE", "", FETCH_CREDENTIALS_MODE_OMIT, "DELETE", "",
       FETCH_CREDENTIALS_MODE_OMIT, true},

      // Methods not in cached preflight response:
      {"GET", "", FETCH_CREDENTIALS_MODE_OMIT, "PUT", "",
       FETCH_CREDENTIALS_MODE_OMIT, false},
      {"GET, POST, DELETE", "", FETCH_CREDENTIALS_MODE_OMIT, "PUT", "",
       FETCH_CREDENTIALS_MODE_OMIT, false},

      // Allowed headers:
      {"GET", "X-MY-HEADER", FETCH_CREDENTIALS_MODE_OMIT, "GET",
       "X-MY-HEADER:t", FETCH_CREDENTIALS_MODE_OMIT, true},
      {"GET", "X-MY-HEADER, Y-MY-HEADER", FETCH_CREDENTIALS_MODE_OMIT, "GET",
       "X-MY-HEADER:t\r\nY-MY-HEADER:t", FETCH_CREDENTIALS_MODE_OMIT, true},
      {"GET", "x-my-header, Y-MY-HEADER", FETCH_CREDENTIALS_MODE_OMIT, "GET",
       "X-MY-HEADER:t\r\ny-my-header:t", FETCH_CREDENTIALS_MODE_OMIT, true},

      // Requested headers not in cached preflight response:
      {"GET", "", FETCH_CREDENTIALS_MODE_OMIT, "GET", "X-MY-HEADER:t",
       FETCH_CREDENTIALS_MODE_OMIT, false},
      {"GET", "X-SOME-OTHER-HEADER", FETCH_CREDENTIALS_MODE_OMIT, "GET",
       "X-MY-HEADER:t", FETCH_CREDENTIALS_MODE_OMIT, false},
      {"GET", "X-MY-HEADER", FETCH_CREDENTIALS_MODE_OMIT, "GET",
       "X-MY-HEADER:t\r\nY-MY-HEADER:t", FETCH_CREDENTIALS_MODE_OMIT, false},

      // Different credential modes:
      {"GET", "", FETCH_CREDENTIALS_MODE_INCLUDE, "GET", "",
       FETCH_CREDENTIALS_MODE_OMIT, true},
      {"GET", "", FETCH_CREDENTIALS_MODE_INCLUDE, "GET", "",
       FETCH_CREDENTIALS_MODE_INCLUDE, true},

      // Credential mode mismatch:
      {"GET", "", FETCH_CREDENTIALS_MODE_OMIT, "GET", "",
       FETCH_CREDENTIALS_MODE_INCLUDE, false},
      {"GET", "", FETCH_CREDENTIALS_MODE_OMIT, "GET", "",
       FETCH_CREDENTIALS_MODE_PASSWORD, false},
  };

  for (const auto& test : tests) {
    SCOPED_TRACE(
        ::testing::Message()
        << "allow_methods: [" << test.allow_methods << "], allow_headers: ["
        << test.allow_headers << "], request_method: [" << test.request_method
        << "], request_headers: [" << test.request_headers
        << "] expect CanSkipPreflight() to return " << test.can_skip_preflight);

    url::Origin origin;
    GURL url("https://test.tdl");

    std::unique_ptr<CrossOriginPreflightResultCacheItem> item = CreateCacheItem(
        test.allow_methods, test.allow_headers, test.cache_credentials_mode);

    EXPECT_TRUE(item);

    test::TestCrossOriginPreflightResultCache cache;

    cache.AppendEntry(origin, url, std::move(item));

    EXPECT_EQ(cache.CanSkipPreflight(origin, url, test.request_credentials_mode,
                                     test.request_method, test.request_headers),
              test.can_skip_preflight);
  }
}

}  // namespace content
