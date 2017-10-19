// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/child_process_security_policy_impl.h"
#include "content/browser/frame_host/frame_tree_node.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/browser_side_navigation_policy.h"
#include "content/public/common/url_constants.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "content/shell/browser/shell.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

namespace content {

class SecurityPropertiesNavigationBrowserTest : public ContentBrowserTest {
 public:
  SecurityPropertiesNavigationBrowserTest() {}

 protected:
  void SetUpOnMainThread() override {
    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(embedded_test_server()->Start());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(SecurityPropertiesNavigationBrowserTest);
};

// Verify that a chrome: scheme document cannot add iframes to web content.
// See https://crbug.com/683418.
IN_PROC_BROWSER_TEST_F(SecurityPropertiesNavigationBrowserTest,
                       NoWebFrameInChromeScheme) {
  FrameTreeNode* root = static_cast<WebContentsImpl*>(shell()->web_contents())
                            ->GetFrameTree()
                            ->root();

  // TODO(nasko): Replace this URL with one with custom WebUI object that
  // doesn't have restrictive CSP, so the test can successfully add an
  // iframe and test the actual throttle blocking. Otherwise the CSP policy
  // will just block the navigation prior to the throttle being even
  // invoked.
  GURL chrome_url = GURL(std::string(kChromeUIScheme) + "://" +
                         std::string(kChromeUIBlobInternalsHost));
  NavigateToURL(shell(), chrome_url);
  EXPECT_EQ(chrome_url, root->current_frame_host()->GetLastCommittedURL());

  {
    GURL web_url(embedded_test_server()->GetURL("/title2.html"));
    std::string script = base::StringPrintf(
        "var frame = document.createElement('iframe');\n"
        "frame.src = '%s';\n"
        "document.body.appendChild(frame);\n",
        web_url.spec().c_str());

    TestNavigationObserver navigation_observer(shell()->web_contents());
    EXPECT_TRUE(ExecuteScript(shell(), script));
    navigation_observer.Wait();

    EXPECT_FALSE(navigation_observer.last_navigation_succeeded());
  }

  // Navigations to data: URLs is handled entirely in the renderer process
  // without PlzNavigate, so only test this case if enabled.
  if (IsBrowserSideNavigationEnabled()) {
    GURL data_url("data:text/html,foo");
    std::string script = base::StringPrintf(
        "var frame = document.createElement('iframe');\n"
        "frame.src = '%s';\n"
        "document.body.appendChild(frame);\n",
        data_url.spec().c_str());

    TestNavigationObserver navigation_observer(shell()->web_contents());
    EXPECT_TRUE(ExecuteScript(shell(), script));
    navigation_observer.Wait();

    EXPECT_FALSE(navigation_observer.last_navigation_succeeded());
  }

  // Verify that an iframe with "about:blank" URL is actually allowed. Not
  // sure why this could be useful, but from security perspective it can
  // only host content coming from the parent document, so it effectively
  // has the same security context.
  {
    GURL about_blank_url("about:blank");
    std::string script = base::StringPrintf(
        "var frame = document.createElement('iframe');\n"
        "frame.src = '%s';\n"
        "document.body.appendChild(frame);\n",
        about_blank_url.spec().c_str());

    TestNavigationObserver navigation_observer(shell()->web_contents());
    EXPECT_TRUE(ExecuteScript(shell(), script));
    navigation_observer.Wait();

    EXPECT_TRUE(navigation_observer.last_navigation_succeeded());
  }
}

// Verify that no web content can be loaded in a process that has WebUI
// bindings, regardless of what scheme the content was loaded from.
IN_PROC_BROWSER_TEST_F(SecurityPropertiesNavigationBrowserTest,
                       NoWebFrameInWebUIProcess) {
  FrameTreeNode* root = static_cast<WebContentsImpl*>(shell()->web_contents())
                            ->GetFrameTree()
                            ->root();
  GURL data_url("data:text/html,a data url document");
  NavigateToURL(shell(), data_url);
  EXPECT_EQ(data_url, root->current_frame_host()->GetLastCommittedURL());
  EXPECT_FALSE(ChildProcessSecurityPolicyImpl::GetInstance()->HasWebUIBindings(
      root->current_frame_host()->GetProcess()->GetID()));

  // Grant WebUI bindings to the process. This will ensure that if there is
  // a mistake in the navigation logic and a process gets somehow WebUI
  // bindings, it cannot include web content regardless of the scheme of the
  // document.
  ChildProcessSecurityPolicyImpl::GetInstance()->GrantWebUIBindings(
      root->current_frame_host()->GetProcess()->GetID());
  EXPECT_TRUE(ChildProcessSecurityPolicyImpl::GetInstance()->HasWebUIBindings(
      root->current_frame_host()->GetProcess()->GetID()));
  {
    GURL web_url(embedded_test_server()->GetURL("/title2.html"));
    std::string script = base::StringPrintf(
        "var frame = document.createElement('iframe');\n"
        "frame.src = '%s';\n"
        "document.body.appendChild(frame);\n",
        web_url.spec().c_str());

    TestNavigationObserver navigation_observer(shell()->web_contents());
    EXPECT_TRUE(ExecuteScript(shell(), script));
    navigation_observer.Wait();

    EXPECT_EQ(1U, root->child_count());
    EXPECT_FALSE(navigation_observer.last_navigation_succeeded());
  }
}

}  // namespace content
