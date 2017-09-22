// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/run_loop.h"
#include "base/strings/string_util.h"
#include "components/origin_manifest/interfaces/origin_manifest.mojom.h"
#include "components/origin_manifest/origin_manifest_store.h"
#include "components/origin_manifest/origin_manifest_store_client.h"
#include "components/origin_manifest/origin_manifest_store_impl.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
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
class OriginManifestStoreBrowserTest : public ContentBrowserTest {
 protected:
  OriginManifestStoreBrowserTest()
      : server1_(CreateServer()), server2_(nullptr), request_counter(0) {}

  ~OriginManifestStoreBrowserTest() override {}

  void SetUpOnMainThread() override { ASSERT_TRUE(server1()->Start()); }

  void ConnectToStore(
      origin_manifest::mojom::OriginManifestStoreRequest request) {
    shell()
        ->web_contents()
        ->GetBrowserContext()
        ->GetOriginManifestStore()
        ->BindRequest(std::move(request));
  }

  net::EmbeddedTestServer* CreateServer() {
    net::EmbeddedTestServer* server =
        new net::EmbeddedTestServer(net::EmbeddedTestServer::TYPE_HTTPS);
    server->RegisterRequestHandler(
        base::Bind(&OriginManifestStoreBrowserTest::HandleResourceRequest,
                   base::Unretained(this)));
    server->RegisterRequestHandler(
        base::Bind(&OriginManifestStoreBrowserTest::HandleOriginManifestRequest,
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
               base::StartsWith(path, "/confirmManifest1",
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

namespace {

void IsEmptyCallback(base::Closure quit_closure,
                     origin_manifest::mojom::OriginManifestPtr om) {
  EXPECT_TRUE(om.Equals(nullptr));
  std::move(quit_closure).Run();
}

void HasOriginManifestCallback(base::Closure quit_closure,
                               std::string version,
                               origin_manifest::mojom::OriginManifestPtr om) {
  EXPECT_FALSE(om.Equals(nullptr));
  EXPECT_EQ(om->version, version);
  std::move(quit_closure).Run();
}

}  // namespace

// Origin Manifest not supported by server, no manifest cached
IN_PROC_BROWSER_TEST_F(OriginManifestStoreBrowserTest, OnServerNotSupported) {
  origin_manifest::mojom::OriginManifestStorePtr store;
  ConnectToStore(mojo::MakeRequest(&store));
  ASSERT_TRUE(store.is_bound());

  GURL url = server1()->GetURL("/noOriginManifest");
  std::string origin = url.GetOrigin().spec();

  base::RunLoop run_loop1;
  store->Get(origin, base::BindOnce(&IsEmptyCallback, run_loop1.QuitClosure()));
  run_loop1.Run();

  EXPECT_TRUE(NavigateToURL(shell(), url));
  EXPECT_EQ(get_request_counter(), 1);

  base::RunLoop run_loop2;
  store->Get(origin, base::BindOnce(&IsEmptyCallback, run_loop2.QuitClosure()));
  run_loop2.Run();
}

// Fetch Origin Manifest file, no manifest cached
IN_PROC_BROWSER_TEST_F(OriginManifestStoreBrowserTest,
                       FetchOriginManifestNoCache) {
  origin_manifest::mojom::OriginManifestStorePtr store;
  ConnectToStore(mojo::MakeRequest(&store));
  ASSERT_TRUE(store.is_bound());

  GURL url = server1()->GetURL("/manifest1");
  std::string origin = url.GetOrigin().spec();

  base::RunLoop run_loop1;
  store->Get(origin, base::BindOnce(&IsEmptyCallback, run_loop1.QuitClosure()));
  run_loop1.Run();

  EXPECT_TRUE(NavigateToURL(shell(), url));
  EXPECT_EQ(get_request_counter(), 2);

  base::RunLoop run_loop2;
  store->Get(origin, base::BindOnce(&HasOriginManifestCallback,
                                    run_loop2.QuitClosure(), "manifest1"));
  run_loop2.Run();
}

// No fetch on confirmation of cached Origin Manifest version
IN_PROC_BROWSER_TEST_F(OriginManifestStoreBrowserTest, ConfirmCachedVersion) {
  origin_manifest::mojom::OriginManifestStorePtr store;
  ConnectToStore(mojo::MakeRequest(&store));
  ASSERT_TRUE(store.is_bound());

  GURL url = server1()->GetURL("/manifest1");
  std::string version = "manifest1";
  EXPECT_TRUE(NavigateToURL(shell(), url));
  EXPECT_EQ(get_request_counter(), 2);

  // we should have Origin Manifest manifest1 by now
  base::RunLoop run_loop1;
  store->Get(url.GetOrigin().spec(),
             base::BindOnce(&HasOriginManifestCallback, run_loop1.QuitClosure(),
                            version));
  run_loop1.Run();

  url = server1()->GetURL("/confirmManifest1");
  EXPECT_TRUE(NavigateToURL(shell(), url));
  EXPECT_EQ(get_request_counter(), 3);

  base::RunLoop run_loop2;
  store->Get(url.GetOrigin().spec(),
             base::BindOnce(&HasOriginManifestCallback, run_loop2.QuitClosure(),
                            version));
  run_loop2.Run();
}

// Fetch Origin Manifest file, but server does not serve it
IN_PROC_BROWSER_TEST_F(OriginManifestStoreBrowserTest, OriginManifestNotFound) {
  origin_manifest::mojom::OriginManifestStorePtr store;
  ConnectToStore(mojo::MakeRequest(&store));
  ASSERT_TRUE(store.is_bound());

  GURL url = server1()->GetURL("/originManifestNotFound");
  EXPECT_TRUE(NavigateToURL(shell(), url));
  EXPECT_EQ(get_request_counter(), 2);

  base::RunLoop run_loop1;
  store->Get(url.GetOrigin().spec(),
             base::BindOnce(&IsEmptyCallback, run_loop1.QuitClosure()));
  run_loop1.Run();
}

// No fetch on missing Origin Manifest reponse header, manifest cached
IN_PROC_BROWSER_TEST_F(OriginManifestStoreBrowserTest,
                       CachedButOnServerNotSupported) {
  origin_manifest::mojom::OriginManifestStorePtr store;
  ConnectToStore(mojo::MakeRequest(&store));
  ASSERT_TRUE(store.is_bound());

  GURL url = server1()->GetURL("/manifest1");
  std::string version = "manifest1";
  EXPECT_TRUE(NavigateToURL(shell(), url));
  EXPECT_EQ(get_request_counter(), 2);

  // we should have Origin Manifest manifest1 by now
  base::RunLoop run_loop1;
  store->Get(url.GetOrigin().spec(),
             base::BindOnce(&HasOriginManifestCallback, run_loop1.QuitClosure(),
                            version));
  run_loop1.Run();

  url = server1()->GetURL("/noOriginManifestHeader");
  EXPECT_TRUE(NavigateToURL(shell(), url));
  EXPECT_EQ(get_request_counter(), 3);

  base::RunLoop run_loop2;
  store->Get(url.GetOrigin().spec(),
             base::BindOnce(&HasOriginManifestCallback, run_loop2.QuitClosure(),
                            version));
  run_loop2.Run();
}

// Fetch new Origin Manifest file on updated version
IN_PROC_BROWSER_TEST_F(OriginManifestStoreBrowserTest, FetchNewVersion) {
  origin_manifest::mojom::OriginManifestStorePtr store;
  ConnectToStore(mojo::MakeRequest(&store));
  ASSERT_TRUE(store.is_bound());

  GURL url = server1()->GetURL("/manifest1");
  std::string version = "manifest1";
  EXPECT_TRUE(NavigateToURL(shell(), url));
  EXPECT_EQ(get_request_counter(), 2);

  // we should have Origin Manifest manifest1 by now
  base::RunLoop run_loop1;
  store->Get(url.GetOrigin().spec(),
             base::BindOnce(&HasOriginManifestCallback, run_loop1.QuitClosure(),
                            version));
  run_loop1.Run();

  url = server1()->GetURL("/manifest2");
  version = "manifest2";
  EXPECT_TRUE(NavigateToURL(shell(), url));
  EXPECT_EQ(get_request_counter(), 4);

  base::RunLoop run_loop2;
  store->Get(url.GetOrigin().spec(),
             base::BindOnce(&HasOriginManifestCallback, run_loop2.QuitClosure(),
                            version));
  run_loop2.Run();
}

// Revoke Origin Manifest on revocation request
IN_PROC_BROWSER_TEST_F(OriginManifestStoreBrowserTest, RevokeOriginManifest) {
  origin_manifest::mojom::OriginManifestStorePtr store;
  ConnectToStore(mojo::MakeRequest(&store));
  ASSERT_TRUE(store.is_bound());

  GURL url = server1()->GetURL("/manifest1");
  std::string version = "manifest1";
  EXPECT_TRUE(NavigateToURL(shell(), url));
  EXPECT_EQ(get_request_counter(), 2);

  // we should have Origin Manifest manifest1 by now
  base::RunLoop run_loop1;
  store->Get(url.GetOrigin().spec(),
             base::BindOnce(&HasOriginManifestCallback, run_loop1.QuitClosure(),
                            version));
  run_loop1.Run();

  url = server1()->GetURL("/revoke");
  EXPECT_TRUE(NavigateToURL(shell(), url));
  EXPECT_EQ(get_request_counter(), 3);

  base::RunLoop run_loop2;
  store->Get(url.GetOrigin().spec(),
             base::BindOnce(&IsEmptyCallback, run_loop2.QuitClosure()));
  run_loop2.Run();
}

// Fetch Origin Manifest on sub-resource loads just as for navigations
IN_PROC_BROWSER_TEST_F(OriginManifestStoreBrowserTest,
                       FetchOriginManifestOnSubresourceLoad) {
  initServer2();
  EXPECT_TRUE(server2()->Start());

  origin_manifest::mojom::OriginManifestStorePtr store;
  ConnectToStore(mojo::MakeRequest(&store));
  ASSERT_TRUE(store.is_bound());

  GURL url = server1()->GetURL("/subresource");
  GURL subresource = server2()->GetURL("/image.jpg");
  std::string version1 = "manifest1";
  std::string version2 = "manifest2";

  EXPECT_TRUE(NavigateToURL(shell(), url));
  EXPECT_EQ(get_request_counter(), 4);

  // we should have both Origin Manifests manifest1 and manifest2 by now
  base::RunLoop run_loop1;
  store->Get(url.GetOrigin().spec(),
             base::BindOnce(&HasOriginManifestCallback, run_loop1.QuitClosure(),
                            version1));
  store->Get(subresource.GetOrigin().spec(),
             base::BindOnce(&HasOriginManifestCallback, run_loop1.QuitClosure(),
                            version2));
  run_loop1.Run();
}

}  // namespace content
