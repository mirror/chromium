// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "chrome/browser/domain_reliability/service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/domain_reliability/service.h"
#include "net/base/net_errors.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "net/test/url_request/url_request_failed_job.h"
#include "url/gurl.h"

namespace domain_reliability {

class DomainReliabilityBrowserTest : public InProcessBrowserTest {
 public:
  DomainReliabilityBrowserTest() {
    net::URLRequestFailedJob::AddUrlHandler();
  }

  ~DomainReliabilityBrowserTest() override {}

  // Note: In an ideal world, instead of appending the command-line switch and
  // manually setting discard_uploads to false, Domain Reliability would
  // continuously monitor the metrics reporting pref, and the test could just
  // set the pref.

  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(switches::kEnableDomainReliability);
  }

  void SetUpOnMainThread() override {
    DomainReliabilityService* service = GetService();
    if (service)
      service->SetDiscardUploadsForTesting(false);
  }

  DomainReliabilityService* GetService() {
    return DomainReliabilityServiceFactory::GetForBrowserContext(
        browser()->profile());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(DomainReliabilityBrowserTest);
};

class DomainReliabilityDisabledBrowserTest
    : public DomainReliabilityBrowserTest {
 protected:
  DomainReliabilityDisabledBrowserTest() {}

  ~DomainReliabilityDisabledBrowserTest() override {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(switches::kDisableDomainReliability);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(DomainReliabilityDisabledBrowserTest);
};

IN_PROC_BROWSER_TEST_F(DomainReliabilityDisabledBrowserTest,
                       ServiceNotCreated) {
  EXPECT_FALSE(GetService());
}

IN_PROC_BROWSER_TEST_F(DomainReliabilityBrowserTest, ServiceCreated) {
  EXPECT_TRUE(GetService());
}

std::unique_ptr<net::test_server::HttpResponse> TestRequestHandler(
    int* request_count_out,
    std::string* last_request_content_out,
    const base::Closure& quit_closure,
    const net::test_server::HttpRequest& request) {
  ++*request_count_out;
  *last_request_content_out = request.has_content ? request.content : "";

  quit_closure.Run();

  auto response = base::MakeUnique<net::test_server::BasicHttpResponse>();
  response->set_code(net::HTTP_OK);
  response->set_content("");
  response->set_content_type("text/plain");
  return std::move(response);
}

IN_PROC_BROWSER_TEST_F(DomainReliabilityBrowserTest, Upload) {
  DomainReliabilityService* service = GetService();

  base::RunLoop run_loop;

  int request_count = 0;
  std::string last_request_content;
  net::test_server::EmbeddedTestServer test_server(
      (net::test_server::EmbeddedTestServer::TYPE_HTTPS));
  test_server.RegisterRequestHandler(
      base::Bind(&TestRequestHandler, &request_count, &last_request_content,
                 run_loop.QuitClosure()));
  ASSERT_TRUE(test_server.Start());

  auto config = base::MakeUnique<DomainReliabilityConfig>();
  config->origin = GURL("https://localhost/");
  config->include_subdomains = false;
  config->collectors.push_back(base::MakeUnique<GURL>(test_server.base_url()));
  config->success_sample_rate = 1.0;
  config->failure_sample_rate = 1.0;
  service->AddContextForTesting(std::move(config));

  ui_test_utils::NavigateToURL(browser(), GURL("https://localhost/"));

  service->ForceUploadsForTesting();

  run_loop.Run();

  EXPECT_EQ(1, request_count);
  EXPECT_NE("", last_request_content);

  ASSERT_TRUE(test_server.ShutdownAndWaitUntilComplete());
}

IN_PROC_BROWSER_TEST_F(DomainReliabilityBrowserTest, UploadAtShutdown) {
  DomainReliabilityService* service = GetService();

  auto config = base::MakeUnique<DomainReliabilityConfig>();
  config->origin = GURL("https://localhost/");
  config->include_subdomains = false;
  config->collectors.push_back(base::MakeUnique<GURL>(
      net::URLRequestFailedJob::GetMockHttpsUrl(net::ERR_IO_PENDING)));
  config->success_sample_rate = 1.0;
  config->failure_sample_rate = 1.0;
  service->AddContextForTesting(std::move(config));

  ui_test_utils::NavigateToURL(browser(), GURL("https://localhost/"));

  service->ForceUploadsForTesting();

  // At this point, there is an upload pending. If everything goes well, the
  // test will finish, destroy the profile, and Domain Reliability will shut
  // down properly. If things go awry, it may crash as terminating the pending
  // upload calls into already-destroyed parts of the component.
}

}  // namespace domain_reliability
