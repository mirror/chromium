// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/run_loop.h"
#include "content/browser/loader/detached_resource_request.h"
#include "content/browser/loader/resource_dispatcher_host_impl.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/test_utils.h"
#include "content/shell/browser/shell.h"

namespace content {

class DetachedResourceRequestBrowserTest : public ContentBrowserTest {
 public:
  DetachedResourceRequestBrowserTest() {}
  ~DetachedResourceRequestBrowserTest() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(DetachedResourceRequestBrowserTest);
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

  auto* browser_context = shell()->web_contents()->GetBrowserContext();
  GURL url(embedded_test_server()->GetURL("/set-cookie?cookie"));
  GURL site_for_cookies(embedded_test_server()->base_url());

  std::string cookie = content::GetCookies(browser_context, site_for_cookies);
  ASSERT_EQ("", cookie);

  auto detached_request =
      DetachedResourceRequest::Create(browser_context, url, GURL(""));
  ASSERT_TRUE(detached_request);
  DetachedResourceRequest::Start(std::move(detached_request));

  content::RunAllTasksUntilIdle();

  cookie = content::GetCookies(browser_context, url);
  EXPECT_EQ("cookie", cookie);
}

IN_PROC_BROWSER_TEST_F(DetachedResourceRequestBrowserTest,
                       CanSetThirdPartyCookie) {
  ASSERT_TRUE(embedded_test_server()->Start());

  auto* browser_context = shell()->web_contents()->GetBrowserContext();
  GURL url(embedded_test_server()->GetURL("/set-cookie?cookie"));
  GURL site_for_cookies("https://cats.google.com/");

  std::string cookie = content::GetCookies(browser_context, site_for_cookies);
  ASSERT_EQ("", cookie);

  auto detached_request =
      DetachedResourceRequest::Create(browser_context, url, GURL(""));
  ASSERT_TRUE(detached_request);
  DetachedResourceRequest::Start(std::move(detached_request));

  content::RunAllTasksUntilIdle();

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

  std::string cookie = content::GetCookies(browser_context, site_for_cookies);
  ASSERT_EQ("", cookie);

  auto detached_request =
      DetachedResourceRequest::Create(browser_context, url, GURL(""));
  ASSERT_TRUE(detached_request);
  DetachedResourceRequest::Start(std::move(detached_request));

  content::RunAllTasksUntilIdle();

  cookie = content::GetCookies(browser_context, url);
  EXPECT_EQ("cookie", cookie);
}

}  // namespace content
