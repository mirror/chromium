// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/run_loop.h"
#include "content/browser/loader/detached_resource_request.h"
#include "content/browser/loader/resource_dispatcher_host_impl.h"
#include "content/public/browser/resource_dispatcher_host_delegate.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/test_utils.h"
#include "content/shell/browser/shell.h"
#include "net/url_request/url_request.h"

namespace content {

class DetachedResourceRequestBrowserTest : public ContentBrowserTest {
 public:
  DetachedResourceRequestBrowserTest() {}
  ~DetachedResourceRequestBrowserTest() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(DetachedResourceRequestBrowserTest);
};

// Allows for a given request to be processed.
class DetachedResourceDispatcherHostDelegate
    : public ResourceDispatcherHostDelegate {
 public:
  explicit DetachedResourceDispatcherHostDelegate(const GURL& watched_url)
      : run_loop_(), watched_url_(watched_url) {}

  void InstallDelegate() {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(
            [](ResourceDispatcherHostDelegate* delegate) {
              ResourceDispatcherHostImpl::Get()->SetDelegate(delegate);
            },
            base::Unretained(this)));
  }

  void WaitForRequestComplete() {
    CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
    run_loop_.Run();
  }

  // ResourceDispatcheHostDelegate:
  void OnResponseStarted(net::URLRequest* request,
                         ResourceContext* resource_context,
                         network::ResourceResponse* response) override {
    LOG(WARNING) << "OnResponseStarted = " << request->url().spec();
    if (request->url() == watched_url_) {
      ResourceDispatcherHostImpl::Get()->SetDelegate(nullptr);
      BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                              run_loop_.QuitClosure());
    }
  }

 private:
  base::RunLoop run_loop_;
  GURL watched_url_;
};

IN_PROC_BROWSER_TEST_F(DetachedResourceRequestBrowserTest, CanStart) {
  auto* browser_context = shell()->web_contents()->GetBrowserContext();
  auto detached_request =
      DetachedResourceRequest::Create(browser_context, GURL(""), GURL(""));
  ASSERT_TRUE(detached_request);
  DetachedResourceRequest::Start(std::move(detached_request));
}

IN_PROC_BROWSER_TEST_F(DetachedResourceRequestBrowserTest, CanSetCookie) {
  ASSERT_TRUE(embedded_test_server()->Start());

  GURL url(embedded_test_server()->GetURL("/set-cookie?cookie"));
  GURL site_for_cookies(embedded_test_server()->base_url());
  auto delegate = std::make_unique<DetachedResourceDispatcherHostDelegate>(url);
  delegate->InstallDelegate();

  auto* browser_context = shell()->web_contents()->GetBrowserContext();
  std::string cookie = content::GetCookies(browser_context, site_for_cookies);
  ASSERT_EQ("", cookie);

  auto detached_request =
      DetachedResourceRequest::Create(browser_context, url, GURL(""));
  ASSERT_TRUE(detached_request);
  DetachedResourceRequest::Start(std::move(detached_request));
  delegate->WaitForRequestComplete();

  cookie = content::GetCookies(browser_context, url);
  EXPECT_EQ("cookie", cookie);
}

IN_PROC_BROWSER_TEST_F(DetachedResourceRequestBrowserTest,
                       CanSetThirdPartyCookie) {
  ASSERT_TRUE(embedded_test_server()->Start());

  auto* browser_context = shell()->web_contents()->GetBrowserContext();
  GURL url(embedded_test_server()->GetURL("/set-cookie?cookie"));
  GURL site_for_cookies("https://cats.google.com/");

  auto delegate = std::make_unique<DetachedResourceDispatcherHostDelegate>(url);
  delegate->InstallDelegate();

  std::string cookie = content::GetCookies(browser_context, site_for_cookies);
  ASSERT_EQ("", cookie);

  auto detached_request =
      DetachedResourceRequest::Create(browser_context, url, GURL(""));
  ASSERT_TRUE(detached_request);
  DetachedResourceRequest::Start(std::move(detached_request));

  delegate->WaitForRequestComplete();

  cookie = content::GetCookies(browser_context, url);
  EXPECT_EQ("cookie", cookie);
}

IN_PROC_BROWSER_TEST_F(DetachedResourceRequestBrowserTest, FollowsRedirects) {
  ASSERT_TRUE(embedded_test_server()->Start());

  auto* browser_context = shell()->web_contents()->GetBrowserContext();

  GURL redirecting_url(embedded_test_server()->GetURL("/server-redirect?"));
  GURL redirected_url(embedded_test_server()->GetURL("/set-cookie?cookie"));

  GURL url(redirecting_url.spec() + redirected_url.spec());
  GURL site_for_cookies("https://cats.google.com/");

  auto delegate =
      std::make_unique<DetachedResourceDispatcherHostDelegate>(redirected_url);
  ResourceDispatcherHostImpl::Get()->SetDelegate(delegate.get());

  std::string cookie = content::GetCookies(browser_context, site_for_cookies);
  ASSERT_EQ("", cookie);

  auto detached_request =
      DetachedResourceRequest::Create(browser_context, url, GURL(""));
  ASSERT_TRUE(detached_request);
  DetachedResourceRequest::Start(std::move(detached_request));

  delegate->WaitForRequestComplete();

  cookie = content::GetCookies(browser_context, url);
  EXPECT_EQ("cookie", cookie);
}

}  // namespace content
