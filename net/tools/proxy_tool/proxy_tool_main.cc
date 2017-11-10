// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <iostream>
#include <memory>

#include "base/at_exit.h"
#include "base/callback_helpers.h"
#include "base/command_line.h"
#include "base/log_util.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/strings/stringprintf.h"
#include "base/task_scheduler/task_scheduler.h"
#include "base/threading/thread.h"
#include "base/values.h"
#include "build/build_config.h"
#include "net/log/file_net_log_observer.h"
#include "net/log/net_log.h"
#include "net/proxy/dhcp_proxy_script_fetcher_factory.h"
#include "net/proxy/proxy_config_service_fixed.h"
#include "net/proxy/proxy_resolver_error_observer.h"
#include "net/proxy/proxy_resolver_v8_tracing_wrapper.h"
#include "net/proxy/proxy_script_fetcher_impl.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_builder.h"

namespace net {
namespace {

const char kDescription[] =
    "Running diagnostic tests to help debug crbug.com/755537 (DHCP based WPAD "
    "is slow). Trigger the network change, wait for things to finish, and then "
    "attach the logs that were written:";

// Exact values for these request headers shouldn't matter for our purposes.
const char kUserAgent[] =
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like "
    "Gecko) Chrome/60.0.3112.113 Safari/537.36";

const char kAcceptLanguage[] =  "en-us,fr";

const base::FilePath::CharType kNetLogPath[] =
    FILE_PATH_LITERAL("proxy-tool-net-log.json");

const base::FilePath::CharType kLogFilePath[] =
    FILE_PATH_LITERAL("proxy-tool-log.txt");

struct ProxyResolveRequest {
  int index;
  ProxyInfo results;
  ProxyService::PacRequest* request = nullptr;

  void Done(int error) {
    LOG(INFO) << "ProxyResolveRequest::Done [index="  << index << "]";
    std::cerr << "Done proxy resolve " << "[index=" << index << "]" << std::endl;
  }
};

class ProxyTool : public NetworkChangeNotifier::IPAddressObserver {
 public:
  ProxyTool() {
    network_change_notifier_.reset(NetworkChangeNotifier::Create());

    net_log_ = base::WrapUnique(new NetLog());
    url_request_context_ = CreateURLRequestContext(net_log_.get());

    file_net_log_observer_ = net::FileNetLogObserver::CreateUnbounded(
        base::FilePath(kNetLogPath), nullptr);

    file_net_log_observer_->StartObserving(
        net_log_.get(), NetLogCaptureMode::IncludeSocketBytes());

    NetworkChangeNotifier::AddIPAddressObserver(this);

    DoProxyResolve();
  }

  void OnIPAddressChanged() override {
    std::cerr << "IP Address changed" << std::endl;
    logging::ScopedLogDuration log1("ProxyTool::OnIPAddressChanged");
    DoProxyResolve();
  }

 private:
  void DoProxyResolve() {
    int index = num_proxy_resolves_++;
    GURL url = GURL(base::StringPrintf("http://proxy-resolve%d.example.com",
                                           index));

    logging::ScopedLogDuration log1(
        "ProxyTool::DoProxyResolve [index=" + std::to_string(index) + "]");

    std::cerr << "Starting proxy resolve [index=" << index << "]..."
              << std::endl;

    ProxyResolveRequest* request = new ProxyResolveRequest();
    request->index = index;

    url_request_context_->proxy_service()->ResolveProxy(
        url, std::string(), &request->results,
        base::Bind(&ProxyResolveRequest::Done, base::Owned(request)),
        &request->request, nullptr,
        NetLogWithSource::Make(net_log_.get(),
                               NetLogSourceType::PROXY_TOOL_RESOLVE));
  }

  static std::unique_ptr<URLRequestContext> CreateURLRequestContext(
      NetLog* net_log) {
    logging::ScopedLogDuration log1("ProxyTool::CreateURLRequestContext");

    // Will be owned by the URLRequestContext, but need to retain a raw
    // pointer.
    HostResolver* host_resolver =
        HostResolver::CreateDefaultResolver(net_log).release();

    // Will be owned by the URLRequestContext, but need to retain a raw
    // pointer.
    ProxyService* proxy_service = new ProxyService(
        CreateProxyConfigService(),
        base::WrapUnique(new ProxyResolverFactoryV8TracingWrapper(
            host_resolver, net_log, base::Bind([] {
              return std::unique_ptr<ProxyResolverErrorObserver>();
            }))),
        net_log);

    // Build the URLRequestContext.
    URLRequestContextBuilder builder;
    builder.set_user_agent(kUserAgent);
    builder.set_accept_language(kAcceptLanguage);
    builder.set_net_log(net_log);
    builder.set_host_resolver(std::unique_ptr<HostResolver>(host_resolver));
    builder.set_proxy_service(std::unique_ptr<ProxyService>(proxy_service));
    std::unique_ptr<URLRequestContext> url_request_context = builder.Build();

    // Finish initializing the ProxyService now that URLRequestContext exists
    // (as it has circular dependencies for fetching).
    DhcpProxyScriptFetcherFactory dhcp_fetcher_factory;
    std::unique_ptr<net::ProxyScriptFetcher> proxy_script_fetcher =
        std::make_unique<ProxyScriptFetcherImpl>(url_request_context.get());
    proxy_service->SetProxyScriptFetchers(
        std::move(proxy_script_fetcher),
        dhcp_fetcher_factory.Create(url_request_context.get()));

    return url_request_context;
  }

  static std::unique_ptr<ProxyConfigService> CreateProxyConfigService() {
    // Don't bother using the system proxy settings on Linux since it is a
    // horrible mess to setup.
#if defined(OS_MACOSX) || defined(OS_WIN)
      LOG(INFO) << "Creating system proxy config service";
      return ProxyService::CreateSystemProxyConfigService(
          base::ThreadTaskRunnerHandle::Get());
#else
      LOG(INFO) << "Creating fixed proxy config service (auto-detect)";
      ProxyConfig proxy_config;
      proxy_config.set_auto_detect(true);
      return std::make_unique<ProxyConfigServiceFixed>(proxy_config);
#endif
    }

    std::unique_ptr<NetworkChangeNotifier> network_change_notifier_;
    std::unique_ptr<NetLog> net_log_;
    std::unique_ptr<FileNetLogObserver> file_net_log_observer_;
    std::unique_ptr<URLRequestContext> url_request_context_;

    int num_proxy_resolves_ = 0;
};

ProxyTool* g_tool = nullptr;

void StartToolOnIOThread() {
  std::cerr << kDescription << "\n\n"
            << "  " << kLogFilePath << "\n"
            << "  " << kNetLogPath << std::endl;

  // OK to leak this as the program doesn't support graceful shutdown.
  g_tool = new ProxyTool();
}

int Main(int argc, char** argv) {
  base::AtExitManager at_exit_manager;
  base::TaskScheduler::CreateAndStartWithDefaultParams("proxy_tool");
  base::ScopedClosureRunner cleanup(
      base::BindOnce([] { base::TaskScheduler::GetInstance()->Shutdown(); }));
  if (!base::CommandLine::Init(argc, argv)) {
    std::cerr << "ERROR in CommandLine::Init" << std::endl;
    return 1;
  }
  logging::LoggingSettings settings;
  settings.logging_dest = logging::LOG_TO_FILE;
  settings.log_file = kLogFilePath;
  settings.lock_log = logging::LOCK_LOG_FILE;
  settings.delete_old = logging::DELETE_OLD_LOG_FILE;
  logging::InitLogging(settings);

  // Start an IO thread.
  base::Thread io_thread("IO_Thread");
  base::Thread::Options options;
  options.message_loop_type = base::MessageLoop::TYPE_IO;
  CHECK(io_thread.StartWithOptions(options));

  // Start the application main loop.
  base::MessageLoop main_loop;
  io_thread.task_runner()->PostTask(FROM_HERE, base::Bind(&net::StartToolOnIOThread));
  base::RunLoop().Run();

  return 0;
}

}  // namespace
}  // namespace net

int main(int argc, char** argv) {
  return net::Main(argc, argv);
}
