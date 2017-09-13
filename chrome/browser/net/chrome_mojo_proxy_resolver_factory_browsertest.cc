// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/chrome_mojo_proxy_resolver_factory.h"

#include <string>

#include "base/macros.h"
#include "base/run_loop.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/browser_thread.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "net/base/host_port_pair.h"
#include "net/interfaces/proxy_resolver_service.mojom.h"
#include "net/proxy/proxy_info.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

const char kPacScript[] =
    "function FindProxyForURL(url, host) {"
    "  return 'PROXY 1.2.3.4:5';"
    "}";
const char kPacHostPortPair[] = "1.2.3.4:5";

// ProxyResolverRequestClient that can waits until a request is complete and
// report the resulting ProxyInfo.
class TestProxyResolverRequestClient
    : public net::interfaces::ProxyResolverRequestClient {
 public:
  explicit TestProxyResolverRequestClient(
      net::interfaces::ProxyResolverRequestClientRequest request)
      : binding_(this, std::move(request)) {}
  ~TestProxyResolverRequestClient() override {}

  net::ProxyInfo WaitForResult() {
    run_loop_.Run();
    return proxy_info_;
  }

  // net::interfaces::ProxyResolverRequestClient implementation:

  void ReportResult(int32_t error, const net::ProxyInfo& proxy_info) override {
    proxy_info_ = proxy_info;
    run_loop_.Quit();
  }

  void Alert(const std::string& error) override { NOTREACHED(); }

  void OnError(int32_t line_number, const std::string& error) override {
    NOTREACHED();
  }

  void ResolveDns(
      std::unique_ptr<net::HostResolver::RequestInfo> request_info,
      net::interfaces::HostResolverRequestClientPtr client) override {
    NOTREACHED();
  }

 private:
  mojo::Binding<net::interfaces::ProxyResolverRequestClient> binding_;
  net::ProxyInfo proxy_info_;
  base::RunLoop run_loop_;

  DISALLOW_COPY_AND_ASSIGN(TestProxyResolverRequestClient);
};

}  // namespace

class ChromeMojoProxyResolverFactoryBrowserTest : public InProcessBrowserTest {
 public:
  ChromeMojoProxyResolverFactoryBrowserTest() {
    // This both speeds up tests, and makes it so that tests will likely fail if
    // the idle timer is ever started unexpectedly.
    ChromeMojoProxyResolverFactory::SetUtilityProcessIdleTimeoutSecsForTesting(
        0);
  }

  ~ChromeMojoProxyResolverFactoryBrowserTest() override {}

  // Creates and returns a ProxyResolver created via
  // ChromeMojoProxyResolverFactory::GetInstance()->CreateFactoryInterface().
  net::interfaces::ProxyResolverFactoryPtr CreateFactory() {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

    net::interfaces::ProxyResolverFactoryPtr proxy_resolver_factory;
    net::interfaces::ProxyResolverFactoryPtrInfo proxy_resolver_factory_info;
    base::RunLoop run_loop;
    content::BrowserThread::PostTaskAndReply(
        content::BrowserThread::IO, FROM_HERE,
        base::BindOnce(
            &ChromeMojoProxyResolverFactoryBrowserTest::CreateFactoryOnIO,
            base::Unretained(&proxy_resolver_factory_info)),
        run_loop.QuitClosure());
    run_loop.Run();
    proxy_resolver_factory.Bind(std::move(proxy_resolver_factory_info));
    return proxy_resolver_factory;
  }

  static void CreateFactoryOnIO(net::interfaces::ProxyResolverFactoryPtrInfo*
                                    proxy_resolver_factory_info) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
    net::interfaces::ProxyResolverFactoryPtr factory =
        ChromeMojoProxyResolverFactory::GetInstance()->CreateFactoryInterface();
    // Need to use PassInterface() is detach the pipe from the IO thread.
    *proxy_resolver_factory_info = factory.PassInterface();
  }

  // Returns true if the ChromeMojoProxyResolverFactory has factory running in a
  // utility process.
  bool UtilityFactoryExists() {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

    bool utility_factory_exists;
    base::RunLoop run_loop;
    content::BrowserThread::PostTaskAndReply(
        content::BrowserThread::IO, FROM_HERE,
        base::BindOnce(&ChromeMojoProxyResolverFactoryBrowserTest::
                           UtilityFactoryExistsOnIO,
                       base::Unretained(&utility_factory_exists)),
        run_loop.QuitClosure());
    run_loop.Run();
    return utility_factory_exists;
  }

  static void UtilityFactoryExistsOnIO(bool* utility_factory_exists) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
    *utility_factory_exists = !!ChromeMojoProxyResolverFactory::GetInstance()
                                    ->get_utility_factory_for_testing();
  }

  // Creates a ProxyResolver using the provided factory and the script in
  // kPacScript.
  net::interfaces::ProxyResolverPtr CreateResolver(
      net::interfaces::ProxyResolverFactory* factory) {
    net::interfaces::ProxyResolverPtr proxy_resolver;
    net::interfaces::ProxyResolverFactoryRequestClientPtr
        proxy_resolver_factory_client;
    // This is never used by the pac script, so no need to actually bind
    // anything to the client end of this pipe.
    proxy_resolver_factory_request_client_requests_.push_back(
        mojo::MakeRequest(&proxy_resolver_factory_client));
    factory->CreateResolver(kPacScript, mojo::MakeRequest(&proxy_resolver),
                            std::move(proxy_resolver_factory_client));
    return proxy_resolver;
  }

  // Runs a ProxyResolver request, and expects it to succeed and return
  // kPacHostPortPair.
  void RunRequest(net::interfaces::ProxyResolver* proxy_resolver) {
    net::interfaces::ProxyResolverRequestClientPtr proxy_resolver_client;
    TestProxyResolverRequestClient test_client(
        mojo::MakeRequest(&proxy_resolver_client));
    proxy_resolver->GetProxyForUrl(GURL("http://foo/"),
                                   std::move(proxy_resolver_client));
    net::ProxyInfo proxy_info = test_client.WaitForResult();
    ASSERT_EQ(1u, proxy_info.proxy_list().size());
    EXPECT_EQ(kPacHostPortPair,
              proxy_info.proxy_list().Get().host_port_pair().ToString());

    // The utility factory should exist after a successful request.
    EXPECT_TRUE(UtilityFactoryExists());
  }

  // Requests for a ProxyResolverFactoryRequestClient. The client is never used
  // by the provided PAC script, but the ProxyResolver gets made if they're
  // destroyed, so just keep them around in limbo.
  std::vector<net::interfaces::ProxyResolverFactoryRequestClientRequest>
      proxy_resolver_factory_request_client_requests_;
};

// Test that the PAC utility process is shut down when the last ProxyResolver
// goes away, and can be restarted afterwards.
IN_PROC_BROWSER_TEST_F(ChromeMojoProxyResolverFactoryBrowserTest,
                       ShutDownUtilityProcessOnNoResolvers) {
  EXPECT_FALSE(UtilityFactoryExists());

  // Create a factory, a resolver, and run a request, which should result in
  // starting the utility process.
  net::interfaces::ProxyResolverFactoryPtr factory = CreateFactory();
  net::interfaces::ProxyResolverPtr proxy_resolver =
      CreateResolver(factory.get());
  RunRequest(proxy_resolver.get());

  // Claim (incorrectly) that the ProxyResolver was destroyed. This should
  // result in shutting down the utility process, which causes the still live
  // ProxyResolver pipe to be closed.
  base::RunLoop run_loop;
  proxy_resolver.set_connection_error_handler(run_loop.QuitClosure());
  factory->OnResolverDestroyed();
  run_loop.Run();
  EXPECT_FALSE(UtilityFactoryExists());

  // Creating another ProxyResolver should create another utility process.
  proxy_resolver = CreateResolver(factory.get());
  RunRequest(proxy_resolver.get());
}

// Same as above, but creates two ProxyResolvers.
IN_PROC_BROWSER_TEST_F(ChromeMojoProxyResolverFactoryBrowserTest,
                       ShutDownUtilityProcessOnNoResolversWithTwoResolvers) {
  EXPECT_FALSE(UtilityFactoryExists());

  // Create a factory, a resolver, and run a request, which should result in
  // starting the utility process.
  net::interfaces::ProxyResolverFactoryPtr factory = CreateFactory();
  net::interfaces::ProxyResolverPtr proxy_resolver =
      CreateResolver(factory.get());
  net::interfaces::ProxyResolverPtr proxy_resolver2 =
      CreateResolver(factory.get());

  // Claim one resolver was destroyed. Process should still exist, and both
  // resolvers should still work.
  factory->OnResolverDestroyed();

  RunRequest(proxy_resolver.get());
  RunRequest(proxy_resolver2.get());

  // Claim the other resolver was destroyed. Process should now be destroyed,
  // and both ProxyResolvers should get connection errors.
  base::RunLoop run_loop;
  base::RunLoop run_loop2;
  proxy_resolver.set_connection_error_handler(run_loop.QuitClosure());
  proxy_resolver2.set_connection_error_handler(run_loop2.QuitClosure());
  factory->OnResolverDestroyed();
  run_loop.Run();
  run_loop2.Run();

  EXPECT_FALSE(UtilityFactoryExists());

  // Creating another ProxyResolver should create another utility process.
  proxy_resolver = CreateResolver(factory.get());
  RunRequest(proxy_resolver.get());
}

// Check that the utility process will also be shut down if the only
// ProxyFactory with active ProxyResolvers is destroyed.
IN_PROC_BROWSER_TEST_F(ChromeMojoProxyResolverFactoryBrowserTest,
                       ShutDownUtilityProcessOnFactoryDestroyed) {
  EXPECT_FALSE(UtilityFactoryExists());

  // Create a factory, resolvers (Use two, to increase chance of counting
  // errors), and run requests, which should result in starting the utility
  // process.
  net::interfaces::ProxyResolverFactoryPtr factory = CreateFactory();
  net::interfaces::ProxyResolverPtr proxy_resolver =
      CreateResolver(factory.get());
  net::interfaces::ProxyResolverPtr proxy_resolver2 =
      CreateResolver(factory.get());
  RunRequest(proxy_resolver.get());
  RunRequest(proxy_resolver2.get());

  // Destroying the factory should result in a utility process being destroyed,
  // even if it has live ProxyResolvers.
  base::RunLoop run_loop;
  base::RunLoop run_loop2;
  proxy_resolver.set_connection_error_handler(run_loop.QuitClosure());
  proxy_resolver2.set_connection_error_handler(run_loop2.QuitClosure());
  factory.reset();
  run_loop.Run();
  run_loop2.Run();
  EXPECT_FALSE(UtilityFactoryExists());

  // Creating another ProxyResolverFactory and ProxyResolver should create
  // another utility process.
  factory = CreateFactory();
  proxy_resolver = CreateResolver(factory.get());
  RunRequest(proxy_resolver.get());
}

// Same as above, but with two ProxyResolverFactories with live ProxyResolvers.
IN_PROC_BROWSER_TEST_F(ChromeMojoProxyResolverFactoryBrowserTest,
                       ShutDownUtilityProcessOnFactoryDestroyedTwoFactories) {
  EXPECT_FALSE(UtilityFactoryExists());

  net::interfaces::ProxyResolverFactoryPtr factory = CreateFactory();
  net::interfaces::ProxyResolverFactoryPtr factory2 = CreateFactory();
  net::interfaces::ProxyResolverPtr proxy_resolver =
      CreateResolver(factory.get());
  net::interfaces::ProxyResolverPtr proxy_resolver2 =
      CreateResolver(factory2.get());
  // Reverse order of creation, just to make sure both resolvers are live.
  RunRequest(proxy_resolver2.get());
  RunRequest(proxy_resolver.get());

  // Destroying one factory should not destroy the utility process.
  factory.reset();
  RunRequest(proxy_resolver2.get());

  // Destroying the second factory should result in destroying the utility
  // process.
  base::RunLoop run_loop;
  proxy_resolver2.set_connection_error_handler(run_loop.QuitClosure());
  factory2.reset();
  run_loop.Run();
  EXPECT_FALSE(UtilityFactoryExists());

  // Creating another ProxyResolverFactory and ProxyResolver should create
  // another utility process.
  factory = CreateFactory();
  proxy_resolver = CreateResolver(factory.get());
  RunRequest(proxy_resolver.get());
}
