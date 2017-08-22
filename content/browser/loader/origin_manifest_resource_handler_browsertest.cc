// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/strings/string_util.h"
#include "content/browser/origin_manifest/origin_manifest.h"
#include "content/browser/origin_manifest/origin_manifest_storage.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/shell/browser/shell.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"

namespace content {

namespace {

const std::string manifest1_str = "{}";
const std::string manifest2_str = "{}";

// Tests around Origin Manifest fetching.
class OriginManifestResourceHandlerBrowserTest : public ContentBrowserTest {
 protected:
  OriginManifestResourceHandlerBrowserTest() : request_counter(0) {
    origin_manifest_test_server_.reset(new net::EmbeddedTestServer);
    origin_manifest_test_server_->RegisterRequestHandler(base::Bind(
        &OriginManifestResourceHandlerBrowserTest::HandleResourceRequest,
        base::Unretained(this)));
    origin_manifest_test_server_->RegisterRequestHandler(base::Bind(
        &OriginManifestResourceHandlerBrowserTest::HandleOriginManifestRequest,
        base::Unretained(this)));
  }

  ~OriginManifestResourceHandlerBrowserTest() override {}

  void SetUpOnMainThread() override {
    host_resolver()->AddRule("a.com", "127.0.0.1");
    host_resolver()->AddRule("b.com", "127.0.0.1");
    ASSERT_TRUE(origin_manifest_test_server()->Start());
  }

  net::EmbeddedTestServer* origin_manifest_test_server() const {
    return origin_manifest_test_server_.get();
  }

  void reset_request_counter() { request_counter = 0; }

  int get_request_counter() { return request_counter; }

  // Request handler for ordinary resource requests
  std::unique_ptr<net::test_server::HttpResponse> HandleResourceRequest(
      const net::test_server::HttpRequest& request) {
    std::string path = request.GetURL().path();
    if (base::StartsWith(path, "/originmanifest",
                         base::CompareCase::SENSITIVE)) {
      return nullptr;
    }

    request_counter++;
    EXPECT_NE(request.headers.find("sec-origin-manifest"),
              request.headers.end());

    std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
        new net::test_server::BasicHttpResponse());
    http_response->set_code(net::HTTP_OK);
    http_response->set_content("content does not matter");
    http_response->set_content_type("text/html; charset=utf-8");

    // react differently depending on the request
    if (base::StartsWith(path, "/noOriginManifest",
                         base::CompareCase::SENSITIVE)) {
      // do not send the Origin Manifest response header
      return http_response;
    } else if (base::StartsWith(path, "/manifest1",
                                base::CompareCase::SENSITIVE) ||
               base::StartsWith(path, "/confirmVersion",
                                base::CompareCase::SENSITIVE)) {
      http_response->AddCustomHeader("sec-origin-manifest", "\"manifest1\"");
      return http_response;
    } else if (base::StartsWith(path, "/manifest2",
                                base::CompareCase::SENSITIVE)) {
      http_response->AddCustomHeader("sec-origin-manifest", "\"manifest2\"");
      return http_response;
    } else if (base::StartsWith(path, "/originManifestNotFound",
                                base::CompareCase::SENSITIVE)) {
      http_response->AddCustomHeader("sec-origin-manifest",
                                     "\"manifestNotFound\"");
      return http_response;
    } else if (base::StartsWith(path, "/revoke",
                                base::CompareCase::SENSITIVE)) {
      http_response->AddCustomHeader("sec-origin-manifest", "0");
      return http_response;
    } else if (base::StartsWith(path, "/subresource",
                                base::CompareCase::SENSITIVE)) {
      http_response->AddCustomHeader("sec-origin-manifest", "manifest1");
      std::string subresource =
          origin_manifest_test_server()->GetURL("b.com", "/image.jpg").spec();
      http_response->set_content(
          "<!DOCTYPE html><html><head></head><body><img src=\"" + subresource +
          "\" /></body></html>");
      return http_response;
    } else if (base::StartsWith(path, "/image.jpg",
                                base::CompareCase::SENSITIVE)) {
      http_response->AddCustomHeader("sec-origin-manifest", "manifest2");
      http_response->set_content("");
      http_response->set_content_type("image/jpeg");
      return http_response;
    }

    return nullptr;
  }

  // Request handler for Origin Manifest requests
  std::unique_ptr<net::test_server::HttpResponse> HandleOriginManifestRequest(
      const net::test_server::HttpRequest& request) {
    std::string path = request.GetURL().path();
    if (!base::StartsWith(path, "/originmanifest",
                          base::CompareCase::SENSITIVE)) {
      return nullptr;
    }
    request_counter++;

    // request with the requested manifest
    std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
        new net::test_server::BasicHttpResponse());
    http_response->set_code(net::HTTP_OK);
    http_response->set_content_type("application/manifest+json");

    if (path == "/originmanifest/manifest1.json") {
      http_response->set_content(manifest1_str);
      return http_response;
    } else if (path == "/originmanifest/manifest2.json") {
      http_response->set_content(manifest2_str);
      return http_response;
    }
    if (path == "/originmanifest/manifestNotFound.json") {
      http_response->set_code(net::HTTP_NOT_FOUND);
      return http_response;
    }

    return nullptr;
  }

 private:
  std::unique_ptr<net::test_server::EmbeddedTestServer>
      origin_manifest_test_server_;
  int request_counter;
};

}  // namespace

// Origin Manifest not supported by server, no manifest cached
IN_PROC_BROWSER_TEST_F(OriginManifestResourceHandlerBrowserTest,
                       OnServerNotSupported) {
  GURL url = origin_manifest_test_server()->GetURL("/noOriginManifest");
  std::string origin = url.GetOrigin().spec();
  EXPECT_EQ(OriginManifestStorage::GetVersion(origin), "1");

  EXPECT_TRUE(NavigateToURL(shell(), url));

  EXPECT_EQ(get_request_counter(), 1);
  EXPECT_EQ(OriginManifestStorage::GetVersion(origin), "1");
}

// Fetch Origin Manifest file, no manifest cached
IN_PROC_BROWSER_TEST_F(OriginManifestResourceHandlerBrowserTest,
                       FetchOriginManifestNoCache) {
  GURL url = origin_manifest_test_server()->GetURL("/manifest1");
  std::string origin = url.GetOrigin().spec();
  EXPECT_EQ(OriginManifestStorage::GetVersion(origin), "1");

  EXPECT_TRUE(NavigateToURL(shell(), url));

  EXPECT_EQ(get_request_counter(), 2);
  EXPECT_EQ(OriginManifestStorage::GetVersion(origin), "manifest1");
}

// No fetch on confirmation of cached Origin Manifest version
IN_PROC_BROWSER_TEST_F(OriginManifestResourceHandlerBrowserTest,
                       ConfirmCachedVersion) {
  GURL url = origin_manifest_test_server()->GetURL("/confirmVersion");
  std::string origin = url.GetOrigin().spec();
  OriginManifestStorage::Add(
      origin, "manifest1", OriginManifest::CreateOriginManifest(manifest1_str));
  EXPECT_EQ(OriginManifestStorage::GetVersion(origin), "manifest1");

  EXPECT_TRUE(NavigateToURL(shell(), url));

  EXPECT_EQ(get_request_counter(), 1);
  EXPECT_EQ(OriginManifestStorage::GetVersion(origin), "manifest1");
}

// Fetch Origin Manifest file, but server does not serve it
IN_PROC_BROWSER_TEST_F(OriginManifestResourceHandlerBrowserTest,
                       OriginManifestNotFound) {
  GURL url = origin_manifest_test_server()->GetURL("/originManifestNotFound");
  std::string origin = url.GetOrigin().spec();
  EXPECT_EQ(OriginManifestStorage::GetVersion(origin), "1");

  EXPECT_TRUE(NavigateToURL(shell(), url));

  EXPECT_EQ(OriginManifestStorage::GetVersion(origin), "1");
  EXPECT_EQ(get_request_counter(), 2);
}

// No fetch on missing Origin Manifest reponse header, manifest cached
IN_PROC_BROWSER_TEST_F(OriginManifestResourceHandlerBrowserTest,
                       CachedButOnServerNotSupported) {
  GURL url = origin_manifest_test_server()->GetURL("/noOriginManifestHeader");
  std::string origin = url.GetOrigin().spec();
  OriginManifestStorage::Add(
      origin, "manifest1", OriginManifest::CreateOriginManifest(manifest1_str));
  EXPECT_EQ(OriginManifestStorage::GetVersion(origin), "manifest1");
  const OriginManifest* manifest1 =
      OriginManifestStorage::GetManifest(origin, "manifest1");

  EXPECT_TRUE(NavigateToURL(shell(), url));

  EXPECT_EQ(get_request_counter(), 1);
  EXPECT_EQ(OriginManifestStorage::GetVersion(origin), "manifest1");
  EXPECT_TRUE(manifest1 ==
              OriginManifestStorage::GetManifest(origin, "manifest1"));
}

// Fetch new Origin Manifest file on updated version
IN_PROC_BROWSER_TEST_F(OriginManifestResourceHandlerBrowserTest,
                       FetchNewVersion) {
  GURL url = origin_manifest_test_server()->GetURL("/manifest2");
  std::string origin = url.GetOrigin().spec();
  OriginManifestStorage::Add(
      origin, "manifest1", OriginManifest::CreateOriginManifest(manifest1_str));
  EXPECT_EQ(OriginManifestStorage::GetVersion(origin), "manifest1");
  const OriginManifest* manifest1 =
      OriginManifestStorage::GetManifest(origin, "manifest1");

  EXPECT_TRUE(NavigateToURL(shell(), url));

  EXPECT_EQ(get_request_counter(), 2);
  EXPECT_EQ(OriginManifestStorage::GetVersion(origin), "manifest2");
  EXPECT_TRUE(manifest1 !=
              OriginManifestStorage::GetManifest(origin, "manifest2"));
}

// Revoke Origin Manifest on revocation request
IN_PROC_BROWSER_TEST_F(OriginManifestResourceHandlerBrowserTest,
                       RevokeOriginManifest) {
  GURL url = origin_manifest_test_server()->GetURL("/revoke");
  std::string origin = url.GetOrigin().spec();
  OriginManifestStorage::Add(
      origin, "manifest1", OriginManifest::CreateOriginManifest(manifest1_str));
  EXPECT_EQ(OriginManifestStorage::GetVersion(origin), "manifest1");

  EXPECT_TRUE(NavigateToURL(shell(), url));

  EXPECT_EQ(get_request_counter(), 1);
  EXPECT_EQ(OriginManifestStorage::GetVersion(origin), "1");
}

// Fetch Origin Manifest on sub-resource loads just as for navigations
IN_PROC_BROWSER_TEST_F(OriginManifestResourceHandlerBrowserTest,
                       FetchOriginManifestOnSubresourceLoad) {
  GURL url = origin_manifest_test_server()->GetURL("a.com", "/subresource");
  std::string origin = url.GetOrigin().spec();
  std::string subresource_origin = origin_manifest_test_server()
                                       ->GetURL("b.com", "/image.jpg")
                                       .GetOrigin()
                                       .spec();
  EXPECT_EQ(OriginManifestStorage::GetVersion(origin), "1");
  EXPECT_EQ(OriginManifestStorage::GetVersion(subresource_origin), "1");

  EXPECT_TRUE(NavigateToURL(shell(), url));
  EXPECT_TRUE(WaitForRenderFrameReady(shell()->web_contents()->GetMainFrame()));

  EXPECT_EQ(get_request_counter(), 4);
  EXPECT_EQ(OriginManifestStorage::GetVersion(origin), "manifest1");
  EXPECT_EQ(OriginManifestStorage::GetVersion(subresource_origin), "manifest2");
}

}  // namespace content
