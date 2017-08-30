// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/strings/string_util.h"
#include "content/browser/origin_manifest/origin_manifest.h"
#include "content/browser/origin_manifest/origin_manifest_store.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/shell/browser/shell.h"
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
  OriginManifestResourceHandlerBrowserTest()
      : server1_(CreateServer()), server2_(nullptr), request_counter(0) {}

  ~OriginManifestResourceHandlerBrowserTest() override {}

  void SetUpOnMainThread() override { ASSERT_TRUE(server1()->Start()); }

  net::EmbeddedTestServer* CreateServer() {
    net::EmbeddedTestServer* server =
        new net::EmbeddedTestServer(net::EmbeddedTestServer::TYPE_HTTPS);
    server->RegisterRequestHandler(base::Bind(
        &OriginManifestResourceHandlerBrowserTest::HandleResourceRequest,
        base::Unretained(this)));
    server->RegisterRequestHandler(base::Bind(
        &OriginManifestResourceHandlerBrowserTest::HandleOriginManifestRequest,
        base::Unretained(this)));
    server->SetSSLConfig(net::EmbeddedTestServer::CERT_OK);
    return server;
  }

  net::EmbeddedTestServer* server1() const { return server1_.get(); }
  net::EmbeddedTestServer* server2() const { return server2_.get(); }

  void initServer2() { server2_.reset(CreateServer()); }

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
      EXPECT_TRUE(server2()->Started());
      http_response->AddCustomHeader("sec-origin-manifest", "manifest1");
      std::string subresource = server2()->GetURL("/image.jpg").spec();
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
  std::unique_ptr<net::test_server::EmbeddedTestServer> server1_;
  std::unique_ptr<net::test_server::EmbeddedTestServer> server2_;
  int request_counter;
};

}  // namespace

// Origin Manifest not supported by server, no manifest cached
IN_PROC_BROWSER_TEST_F(OriginManifestResourceHandlerBrowserTest,
                       OnServerNotSupported) {
  GURL url = server1()->GetURL("/noOriginManifest");
  std::string origin = url.GetOrigin().spec();
  OriginManifestStore& store = OriginManifestStore::Get();
  EXPECT_EQ(store.GetVersion(origin), "1");

  EXPECT_TRUE(NavigateToURL(shell(), url));

  EXPECT_EQ(get_request_counter(), 1);
  EXPECT_EQ(store.GetVersion(origin), "1");
}

// Fetch Origin Manifest file, no manifest cached
IN_PROC_BROWSER_TEST_F(OriginManifestResourceHandlerBrowserTest,
                       FetchOriginManifestNoCache) {
  GURL url = server1()->GetURL("/manifest1");
  std::string origin = url.GetOrigin().spec();
  OriginManifestStore& store = OriginManifestStore::Get();
  EXPECT_EQ(store.GetVersion(origin), "1");

  EXPECT_TRUE(NavigateToURL(shell(), url));

  EXPECT_EQ(get_request_counter(), 2);
  EXPECT_EQ(store.GetVersion(origin), "manifest1");
}

// No fetch on confirmation of cached Origin Manifest version
IN_PROC_BROWSER_TEST_F(OriginManifestResourceHandlerBrowserTest,
                       ConfirmCachedVersion) {
  GURL url = server1()->GetURL("/confirmVersion");
  std::string origin = url.GetOrigin().spec();
  std::string version = "manifest1";
  OriginManifestStore& store = OriginManifestStore::Get();
  store.Add(
      origin, version,
      OriginManifest::CreateOriginManifest(origin, version, manifest1_str));
  EXPECT_EQ(store.GetVersion(origin), version);

  EXPECT_TRUE(NavigateToURL(shell(), url));

  EXPECT_EQ(get_request_counter(), 1);
  EXPECT_EQ(store.GetVersion(origin), version);
}

// Fetch Origin Manifest file, but server does not serve it
IN_PROC_BROWSER_TEST_F(OriginManifestResourceHandlerBrowserTest,
                       OriginManifestNotFound) {
  GURL url = server1()->GetURL("/originManifestNotFound");
  std::string origin = url.GetOrigin().spec();
  OriginManifestStore& store = OriginManifestStore::Get();
  EXPECT_EQ(store.GetVersion(origin), "1");

  EXPECT_TRUE(NavigateToURL(shell(), url));

  EXPECT_EQ(store.GetVersion(origin), "1");
  EXPECT_EQ(get_request_counter(), 2);
}

// No fetch on missing Origin Manifest reponse header, manifest cached
IN_PROC_BROWSER_TEST_F(OriginManifestResourceHandlerBrowserTest,
                       CachedButOnServerNotSupported) {
  GURL url = server1()->GetURL("/noOriginManifestHeader");
  std::string origin = url.GetOrigin().spec();
  std::string version = "manifest1";
  OriginManifestStore& store = OriginManifestStore::Get();
  store.Add(
      origin, version,
      OriginManifest::CreateOriginManifest(origin, version, manifest1_str));
  EXPECT_EQ(store.GetVersion(origin), version);
  const OriginManifest* manifest1 = store.GetManifest(origin, version);

  EXPECT_TRUE(NavigateToURL(shell(), url));

  EXPECT_EQ(get_request_counter(), 1);
  EXPECT_EQ(store.GetVersion(origin), version);
  EXPECT_TRUE(manifest1 == store.GetManifest(origin, version));
}

// Fetch new Origin Manifest file on updated version
IN_PROC_BROWSER_TEST_F(OriginManifestResourceHandlerBrowserTest,
                       FetchNewVersion) {
  GURL url = server1()->GetURL("/manifest2");
  std::string origin = url.GetOrigin().spec();
  std::string version1 = "manifest1";
  OriginManifestStore& store = OriginManifestStore::Get();
  store.Add(
      origin, version1,
      OriginManifest::CreateOriginManifest(origin, version1, manifest1_str));
  EXPECT_EQ(store.GetVersion(origin), version1);
  const OriginManifest* manifest1 = store.GetManifest(origin, version1);

  EXPECT_TRUE(NavigateToURL(shell(), url));

  std::string version2 = "manifest2";
  EXPECT_EQ(get_request_counter(), 2);
  EXPECT_EQ(store.GetVersion(origin), version2);
  EXPECT_TRUE(manifest1 != store.GetManifest(origin, version2));
}

// Revoke Origin Manifest on revocation request
IN_PROC_BROWSER_TEST_F(OriginManifestResourceHandlerBrowserTest,
                       RevokeOriginManifest) {
  GURL url = server1()->GetURL("/revoke");
  std::string origin = url.GetOrigin().spec();
  std::string version = "manifest1";
  OriginManifestStore& store = OriginManifestStore::Get();
  store.Add(
      origin, version,
      OriginManifest::CreateOriginManifest(origin, version, manifest1_str));
  EXPECT_EQ(store.GetVersion(origin), version);

  EXPECT_TRUE(NavigateToURL(shell(), url));

  EXPECT_EQ(get_request_counter(), 1);
  EXPECT_EQ(store.GetVersion(origin), "1");
}

// Fetch Origin Manifest on sub-resource loads just as for navigations
IN_PROC_BROWSER_TEST_F(OriginManifestResourceHandlerBrowserTest,
                       FetchOriginManifestOnSubresourceLoad) {
  GURL url = server1()->GetURL("/subresource");
  std::string origin = url.GetOrigin().spec();
  initServer2();
  EXPECT_TRUE(server2()->Start());
  std::string subresource_origin =
      server2()->GetURL("/image.jpg").GetOrigin().spec();
  OriginManifestStore& store = OriginManifestStore::Get();
  EXPECT_EQ(store.GetVersion(origin), "1");
  EXPECT_EQ(store.GetVersion(subresource_origin), "1");

  EXPECT_TRUE(NavigateToURL(shell(), url)) << server1()->GetCertificateName();
  EXPECT_TRUE(WaitForRenderFrameReady(shell()->web_contents()->GetMainFrame()));

  EXPECT_EQ(get_request_counter(), 4);
  EXPECT_EQ(store.GetVersion(origin), "manifest1");
  EXPECT_EQ(store.GetVersion(subresource_origin), "manifest2");
}

}  // namespace content
