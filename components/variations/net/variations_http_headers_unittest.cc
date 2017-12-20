// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/variations/net/variations_http_headers.h"

#include <stddef.h>

#include "base/macros.h"
#include "base/strings/stringprintf.h"
#include "components/variations/variations_http_header_provider.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace variations {

TEST(VariationsHttpHeadersTest, ShouldAppendHeaders) {
  struct {
    const char* url;
    bool should_append_headers;
  } cases[] = {
      {"http://google.com", false},
      {"https://google.com", true},
      {"http://www.google.com", false},
      {"https://www.google.com", true},
      {"http://m.google.com", false},
      {"https://m.google.com", true},
      {"http://google.ca", false},
      {"https://google.ca", true},
      {"http://google.co.uk", false},
      {"https://google.co.uk", true},
      {"http://google.co.uk:8080/", false},
      {"https://google.co.uk:8080/", true},
      {"http://www.google.co.uk:8080/", false},
      {"https://www.google.co.uk:8080/", true},
      {"https://google", false},

      {"http://youtube.com", false},
      {"https://youtube.com", true},
      {"http://www.youtube.com", false},
      {"https://www.youtube.com", true},
      {"http://www.youtube.ca", false},
      {"https://www.youtube.ca", true},
      {"http://www.youtube.co.uk:8080/", false},
      {"https://www.youtube.co.uk:8080/", true},
      {"https://youtube", false},

      {"https://www.yahoo.com", false},

      {"http://ad.doubleclick.net", false},
      {"https://ad.doubleclick.net", true},
      {"https://a.b.c.doubleclick.net", true},
      {"https://a.b.c.doubleclick.net:8081", true},
      {"http://www.doubleclick.com", false},
      {"https://www.doubleclick.com", true},
      {"https://www.doubleclick.org", false},
      {"http://www.doubleclick.net.com", false},
      {"https://www.doubleclick.net.com", false},

      {"http://ad.googlesyndication.com", false},
      {"https://ad.googlesyndication.com", true},
      {"https://a.b.c.googlesyndication.com", true},
      {"https://a.b.c.googlesyndication.com:8080", true},
      {"http://www.doubleclick.edu", false},
      {"http://www.googlesyndication.com.edu", false},
      {"https://www.googlesyndication.com.com", false},

      {"http://www.googleadservices.com", false},
      {"https://www.googleadservices.com", true},
      {"http://www.googleadservices.com:8080", false},
      {"https://www.googleadservices.com:8080", true},
      {"https://www.internal.googleadservices.com", true},
      {"https://www2.googleadservices.com", true},
      {"https://www.googleadservices.org", false},
      {"https://www.googleadservices.com.co.uk", false},

      {"http://WWW.ANDROID.COM", false},
      {"https://WWW.ANDROID.COM", true},
      {"http://www.android.com", false},
      {"https://www.android.com", true},
      {"http://www.doubleclick.com", false},
      {"https://www.doubleclick.com", true},
      {"http://www.doubleclick.net", false},
      {"https://www.doubleclick.net", true},
      {"http://www.ggpht.com", false},
      {"https://www.ggpht.com", true},
      {"http://www.googleadservices.com", false},
      {"https://www.googleadservices.com", true},
      {"http://www.googleapis.com", false},
      {"https://www.googleapis.com", true},
      {"http://www.googlesyndication.com", false},
      {"https://www.googlesyndication.com", true},
      {"http://www.googleusercontent.com", false},
      {"https://www.googleusercontent.com", true},
      {"http://www.googlevideo.com", false},
      {"https://www.googlevideo.com", true},
      {"http://ssl.gstatic.com", false},
      {"https://ssl.gstatic.com", true},
      {"http://www.gstatic.com", false},
      {"https://www.gstatic.com", true},
      {"http://www.ytimg.com", false},
      {"https://www.ytimg.com", true},
      {"https://wwwytimg.com", false},
      {"https://ytimg.com", false},

      {"https://www.android.org", false},
      {"https://www.doubleclick.org", false},
      {"http://www.doubleclick.net", false},
      {"https://www.doubleclick.net", true},
      {"https://www.ggpht.org", false},
      {"https://www.googleadservices.org", false},
      {"https://www.googleapis.org", false},
      {"https://www.googlesyndication.org", false},
      {"https://www.googleusercontent.org", false},
      {"https://www.googlevideo.org", false},
      {"https://ssl.gstatic.org", false},
      {"https://www.gstatic.org", false},
      {"https://www.ytimg.org", false},

      {"http://a.b.android.com", false},
      {"https://a.b.android.com", true},
      {"http://a.b.doubleclick.com", false},
      {"https://a.b.doubleclick.com", true},
      {"http://a.b.doubleclick.net", false},
      {"https://a.b.doubleclick.net", true},
      {"http://a.b.ggpht.com", false},
      {"https://a.b.ggpht.com", true},
      {"http://a.b.googleadservices.com", false},
      {"https://a.b.googleadservices.com", true},
      {"http://a.b.googleapis.com", false},
      {"https://a.b.googleapis.com", true},
      {"http://a.b.googlesyndication.com", false},
      {"https://a.b.googlesyndication.com", true},
      {"http://a.b.googleusercontent.com", false},
      {"https://a.b.googleusercontent.com", true},
      {"http://a.b.googlevideo.com", false},
      {"https://a.b.googlevideo.com", true},
      {"http://ssl.gstatic.com", false},
      {"https://ssl.gstatic.com", true},
      {"http://a.b.gstatic.com", false},
      {"https://a.b.gstatic.com", true},
      {"http://a.b.ytimg.com", false},
      {"https://a.b.ytimg.com", true},
      {"http://googleweblight.com", false},
      {"https://googleweblight.com", true},
      {"http://wwwgoogleweblight.com", false},
      {"https://www.googleweblight.com", false},
      {"https://a.b.googleweblight.com", false},
  };

  for (size_t i = 0; i < arraysize(cases); ++i) {
    const GURL url(cases[i].url);
    EXPECT_EQ(cases[i].should_append_headers,
              internal::ShouldAppendVariationHeaders(url))
        << url;
  }
}

TEST(VariationsHttpHeadersTest, TestStrippingHeaders) {
  content::TestBrowserThreadBundle thread_bundle(
      content::TestBrowserThreadBundle::IO_MAINLOOP);

  // Send all network requests to localhost.
  net::MockHostResolver host_resolver;
  host_resolver.rules()->AddRule("*:*", "127.0.0.1");

  // Set up a context with the test network delegate and host resolver
  // so that we can intercept traffic.
  net::TestNetworkDelegate default_network_delegate;
  net::TestURLRequestContext default_context(true);
  default_context.set_network_delegate(&default_network_delegate);
  default_context.set_host_resolver(&host_resolver);
  // Disable HTTPS certificate validation as localhost does not present a valid
  // certificate for https://www.google.com.
  auto params = base::MakeUnique<net::HttpNetworkSession::Params>();
  params->ignore_certificate_errors = true;
  default_context.set_http_network_session_params(std::move(params));
  // Execute the same header stripping as the ChromeNetworkDelegate will in
  // production.
  class StrippingDelegate : public net::NetworkDelegateImpl {
    void OnBeforeRedirect(net::URLRequest* request,
                          const GURL& new_location) override {
      variations::StripVariationHeaderIfNeeded(request, new_location);
    }
  } stripping_delegate;
  default_context.set_network_delegate(&stripping_delegate);
  default_context.Init();

  // Set up a test server that redirects according to the following redirect
  // chain:
  // https://www.google.com:<port>/redirect
  // --> https://www.google.com:<port>/redirect2
  // --> https://www.example.com:<port>/
  net::test_server::EmbeddedTestServer test_server(
      net::test_server::EmbeddedTestServer::TYPE_HTTPS);
  auto handler = [](net::test_server::EmbeddedTestServer* server,
                    const net::test_server::HttpRequest& request)
      -> std::unique_ptr<net::test_server::HttpResponse> {
    auto http_response =
        std::make_unique<net::test_server::BasicHttpResponse>();
    if (request.relative_url == "/redirect") {
      http_response->set_code(net::HTTP_MOVED_PERMANENTLY);
      http_response->AddCustomHeader(
          "Location", base::StringPrintf("https://www.google.com:%d/redirect2",
                                         server->port()));
    } else if (request.relative_url == "/redirect2") {
      http_response->set_code(net::HTTP_MOVED_PERMANENTLY);
      http_response->AddCustomHeader(
          "Location",
          base::StringPrintf("https://www.example.com:%d/", server->port()));
    } else if (request.relative_url == "/") {
      http_response->set_code(net::HTTP_OK);
      http_response->set_content("hello");
      http_response->set_content_type("text/plain");
    } else {
      NOTREACHED();
    }
    return http_response;
  };
  test_server.RegisterRequestHandler(
      base::BindRepeating(handler, base::Unretained(&test_server)));
  ASSERT_TRUE(test_server.Start());

  net::TestDelegate test_delegate;
  test_delegate.set_quit_on_redirect(true);

  // Set up some fake variations.
  auto* variations_provider =
      variations::VariationsHttpHeaderProvider::GetInstance();
  variations_provider->SetDefaultVariationIds({"12", "456", "t789"});

  // Build network request with variations header.
  GURL test_url(base::StringPrintf("https://www.google.com:%d/redirect",
                                   test_server.port()));
  std::unique_ptr<net::URLRequest> req(default_context.CreateRequest(
      test_url, net::DEFAULT_PRIORITY, &test_delegate,
      TRAFFIC_ANNOTATION_FOR_TESTS));
  net::HttpRequestHeaders headers;
  variations::AppendVariationHeaders(test_url, variations::InIncognito::kNo,
                                     variations::SignedIn::kYes, &headers);
  req->SetExtraRequestHeaders(headers);

  // Trigger network request and wait for the response for
  // https://www.google.com:<port>/redirect.
  req->Start();
  base::RunLoop().Run();

  EXPECT_TRUE(test_delegate.have_full_request_headers());
  EXPECT_TRUE(test_delegate.full_request_headers().HasHeader("X-Client-Data"));

  // Follow the redirect to https://www.google.com:<port>/redirect2 and wait
  // for response.
  test_delegate.ClearFullRequestHeaders();
  req->FollowDeferredRedirect();
  base::RunLoop().Run();

  EXPECT_TRUE(test_delegate.have_full_request_headers());
  EXPECT_TRUE(test_delegate.full_request_headers().HasHeader("X-Client-Data"));

  // Follow the redirect to https://www.example.com:<port>/ where X-Client-Data
  // header should be gone.
  test_delegate.ClearFullRequestHeaders();
  req->FollowDeferredRedirect();
  base::RunLoop().Run();

  EXPECT_TRUE(test_delegate.have_full_request_headers());
  EXPECT_FALSE(test_delegate.full_request_headers().HasHeader("X-Client-Data"));

  EXPECT_EQ(net::OK, test_delegate.request_status());
}

}  // namespace variations
