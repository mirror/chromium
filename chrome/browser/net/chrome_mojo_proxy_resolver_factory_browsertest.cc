// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <map>
#include <vector>

#include "base/macros.h"
#include "base/process/process.h"
#include "build/build_config.h"
#include "chrome/browser/net/chrome_mojo_proxy_resolver_factory.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/service_manager_connection.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/proxy_resolver/public/interfaces/proxy_resolver.mojom.h"

namespace {

constexpr char PAC_SCRIPT[] = "PROXY proxy.example.com:1; DIRECT";

constexpr uint32_t kLongTimeoutInSeconds = 15;

constexpr uint32_t kShortTimeoutInSeconds = 2;

// An implementation of ServiceManagerListener that tracks creation/destruction
// of the proxy resolver service.
class TestServiceManagerListener
    : public service_manager::mojom::ServiceManagerListener {
 public:
  TestServiceManagerListener() = default;

  uint32_t pid() const { return pid_; }

  bool WaitUntilServiceStarted(uint32_t timeout_in_s) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    DCHECK(!on_service_event_loop_closure_);
    if (IsServiceRunning())
      return true;
    base::RunLoop run_loop;
    on_service_event_loop_closure_ = run_loop.QuitClosure();
    PostTimeoutRunLoopClosure(run_loop.QuitClosure(), timeout_in_s);
    run_loop.Run();
    on_service_event_loop_closure_.Reset();
    return IsServiceRunning();
  }

  bool WaitUntilServiceStopped(uint32_t timeout_in_s) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    DCHECK(!on_service_event_loop_closure_);
    if (!IsServiceRunning())
      return true;
    base::RunLoop run_loop;
    on_service_event_loop_closure_ = run_loop.QuitClosure();
    PostTimeoutRunLoopClosure(run_loop.QuitClosure(), timeout_in_s);
    run_loop.Run();
    on_service_event_loop_closure_.Reset();
    return !IsServiceRunning();
  }

 private:
  bool IsServiceRunning() { return pid_ != static_cast<uint32_t>(-1); }

  void PostTimeoutRunLoopClosure(base::Closure run_loop_closure,
                                 uint32_t timeout_in_s) {
    content::BrowserThread::PostDelayedTask(
        content::BrowserThread::UI, FROM_HERE, run_loop_closure,
        base::TimeDelta::FromSeconds(timeout_in_s));
  }

  // service_manager::mojom::ServiceManagerListener implementation:
  void OnInit(std::vector<service_manager::mojom::RunningServiceInfoPtr>
                  running_services) override {}

  void OnServiceCreated(
      service_manager::mojom::RunningServiceInfoPtr service) override {}

  void OnServiceStarted(const service_manager::Identity& identity,
                        uint32_t pid) override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    if (identity.name() != proxy_resolver::mojom::kProxyResolverServiceName)
      return;

    DCHECK_EQ(pid_, static_cast<uint32_t>(-1));
    pid_ = pid;
    if (on_service_event_loop_closure_)
      std::move(on_service_event_loop_closure_).Run();
  }

  void OnServicePIDReceived(const service_manager::Identity& identity,
                            uint32_t pid) override {}
  void OnServiceFailedToStart(
      const service_manager::Identity& identity) override {}

  void OnServiceStopped(const service_manager::Identity& identity) override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    if (identity.name() != proxy_resolver::mojom::kProxyResolverServiceName)
      return;

    DCHECK_NE(pid_, static_cast<uint32_t>(-1));
    pid_ = -1;
    if (on_service_event_loop_closure_)
      std::move(on_service_event_loop_closure_).Run();
  }

  uint32_t pid_ = -1;
  base::Closure on_service_event_loop_closure_;
};

void CreateResolverOnIOThread(
    ChromeMojoProxyResolverFactory* factory,
    std::unique_ptr<base::ScopedClosureRunner>* resolver_closure,
    mojo::Binding<proxy_resolver::mojom::ProxyResolverFactoryRequestClient>*
        resolver_client_binding,
    mojo::InterfacePtr<proxy_resolver::mojom::ProxyResolver>* resolver) {
  base::RunLoop run_loop;
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::Bind(
          [](ChromeMojoProxyResolverFactory* factory,
             std::unique_ptr<base::ScopedClosureRunner>* resolver_closure,
             mojo::Binding<
                 proxy_resolver::mojom::ProxyResolverFactoryRequestClient>*
                 resolver_client_binding,
             mojo::InterfacePtr<proxy_resolver::mojom::ProxyResolver>* resolver,
             const base::Closure& loop_closure) {
            proxy_resolver::mojom::ProxyResolverFactoryRequestClientPtr
                factory_client;
            resolver_client_binding->Bind(mojo::MakeRequest(&factory_client));
            *resolver_closure =
                factory->CreateResolver(PAC_SCRIPT, mojo::MakeRequest(resolver),
                                        std::move(factory_client));
            loop_closure.Run();
          },
          factory, resolver_closure, resolver_client_binding, resolver,
          run_loop.QuitClosure()));
  run_loop.Run();
}

void CloseResolverOnIOThread(
    mojo::Binding<proxy_resolver::mojom::ProxyResolverFactoryRequestClient>*
        resolver_client_binding,
    mojo::InterfacePtr<proxy_resolver::mojom::ProxyResolver>* resolver,
    std::unique_ptr<base::ScopedClosureRunner> resolver_closure) {
  base::RunLoop run_loop;
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::Bind(
          [](mojo::Binding<
                 proxy_resolver::mojom::ProxyResolverFactoryRequestClient>*
                 resolver_client_binding,
             mojo::InterfacePtr<proxy_resolver::mojom::ProxyResolver>* resolver,
             std::unique_ptr<base::ScopedClosureRunner> resolver1_closure,
             const base::Closure& loop_closure) {
            resolver_client_binding->Close();
            resolver->reset();
            loop_closure.Run();
          },
          resolver_client_binding, resolver, base::Passed(&resolver_closure),
          run_loop.QuitClosure()));
  run_loop.Run();
}

class DumbProxyResolverFactoryRequestClient
    : public proxy_resolver::mojom::ProxyResolverFactoryRequestClient {
 public:
  DumbProxyResolverFactoryRequestClient() = default;

 private:
  void ReportResult(int32_t error) override {}
  void Alert(const std::string& error) override {}
  void OnError(int32_t line_number, const std::string& error) override {}
  void ResolveDns(
      std::unique_ptr<net::HostResolver::RequestInfo> request_info,
      ::net::interfaces::HostResolverRequestClientPtr client) override {}
};

class ChromeMojoProxyResolverFactoryBrowserTest : public InProcessBrowserTest {
 protected:
  ChromeMojoProxyResolverFactoryBrowserTest() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(ChromeMojoProxyResolverFactoryBrowserTest);
};

}  // namespace
// Ensures the proxy_resolver service is started correctly and stopped when no
// resolvers are open.
IN_PROC_BROWSER_TEST_F(ChromeMojoProxyResolverFactoryBrowserTest,
                       ServiceLifecycle) {
  // Access the service manager so we can set-up a listener for service
  // creation/destruction.
  mojo::InterfacePtr<service_manager::mojom::ServiceManager> service_manager;
  content::ServiceManagerConnection::GetForProcess()
      ->GetConnector()
      ->BindInterface(service_manager::mojom::kServiceName, &service_manager);

  service_manager::mojom::ServiceManagerListenerPtr listener_ptr;
  TestServiceManagerListener listener;
  mojo::Binding<service_manager::mojom::ServiceManagerListener>
      listener_binding(&listener, mojo::MakeRequest(&listener_ptr));
  service_manager->AddListener(std::move(listener_ptr));

  ChromeMojoProxyResolverFactory* factory = nullptr;
  // Create the ChromeMojoProxyResolverFactory.
  {
    base::RunLoop run_loop;
    content::BrowserThread::PostTask(
        content::BrowserThread::IO, FROM_HERE,
        base::Bind(
            [](ChromeMojoProxyResolverFactory** factory,
               const base::Closure& closure) {
              *factory = ChromeMojoProxyResolverFactory::GetInstance();
              (*factory)->SetFactoryIdleTimeoutForTests(1);
              closure.Run();
            },
            &factory, run_loop.QuitClosure()));
    run_loop.Run();
  }

  // Create a resolver, this should create and start the service.
  DumbProxyResolverFactoryRequestClient resolver_client;
  std::unique_ptr<base::ScopedClosureRunner> resolver1_closure;
  mojo::InterfacePtr<proxy_resolver::mojom::ProxyResolver> resolver1;
  mojo::Binding<proxy_resolver::mojom::ProxyResolverFactoryRequestClient>
      resolver1_binding(&resolver_client);
  CreateResolverOnIOThread(factory, &resolver1_closure, &resolver1_binding,
                           &resolver1);
  ASSERT_TRUE(resolver1);
  ASSERT_TRUE(listener.WaitUntilServiceStarted(kLongTimeoutInSeconds));
  uint32_t service_pid = listener.pid();
  uint32_t current_pid = base::Process::Current().Pid();
#if defined(OS_ANDROID)
  // The proxy resolver runs in process on Android.
  ASSERT_EQ(service_pid, current_pid);
#else
  // The proxy resolver runs out process on non Android platforms.
  ASSERT_NE(service_pid, current_pid);
#endif

  // Create anothe resolver.
  std::unique_ptr<base::ScopedClosureRunner> resolver2_closure;
  mojo::InterfacePtr<proxy_resolver::mojom::ProxyResolver> resolver2;
  mojo::Binding<proxy_resolver::mojom::ProxyResolverFactoryRequestClient>
      resolver2_binding(&resolver_client);
  CreateResolverOnIOThread(factory, &resolver2_closure, &resolver2_binding,
                           &resolver2);
  ASSERT_TRUE(resolver2);
  // The service should not be restarted.
  ASSERT_TRUE(listener.WaitUntilServiceStarted(kShortTimeoutInSeconds));

  // Close one resolver.
  CloseResolverOnIOThread(&resolver1_binding, &resolver1,
                          std::move(resolver1_closure));
  // The service should not be stopped as we have another resolver.
  ASSERT_FALSE(listener.WaitUntilServiceStopped(kShortTimeoutInSeconds));

  // Close the last resolver, the service should now go away.
  CloseResolverOnIOThread(&resolver2_binding, &resolver2,
                          std::move(resolver2_closure));
  ASSERT_TRUE(listener.WaitUntilServiceStopped(kLongTimeoutInSeconds));
}
