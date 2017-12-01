// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/macros.h"
#include "base/strings/pattern.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/test/histogram_tester.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/resource_type.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "content/shell/browser/shell.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace content {

// These tests simulate exploited renderer processes, which can fetch arbitrary
// resources from other websites, not constrained by the Same Origin Policy.  We
// are trying to verify that the renderer cannot fetch any cross-site document
// responses even when the Same Origin Policy is turned off inside the renderer.
class CrossSiteDocumentBlockingBaseTest : public ContentBrowserTest {
 public:
  CrossSiteDocumentBlockingBaseTest() {}
  ~CrossSiteDocumentBlockingBaseTest() override {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    // EmbeddedTestServer::InitializeAndListen() initializes its |base_url_|
    // which is required below. This cannot invoke Start() however as that kicks
    // off the "EmbeddedTestServer IO Thread" which then races with
    // initialization in ContentBrowserTest::SetUp(), http://crbug.com/674545.
    ASSERT_TRUE(embedded_test_server()->InitializeAndListen());

    // Add a host resolver rule to map all outgoing requests to the test server.
    // This allows us to use "real" hostnames and standard ports in URLs (i.e.,
    // without having to inject the port number into all URLs), which we can use
    // to create arbitrary SiteInstances.
    command_line->AppendSwitchASCII(
        switches::kHostResolverRules,
        "MAP * " + embedded_test_server()->host_port_pair().ToString() +
            ",EXCLUDE localhost");

    // To test that the renderer process does not receive blocked documents, we
    // disable the same origin policy to let it see cross-origin fetches if they
    // are received.
    command_line->AppendSwitch(switches::kDisableWebSecurity);
  }

  void SetUpOnMainThread() override {
    // Complete the manual Start() after ContentBrowserTest's own
    // initialization, ref. comment on InitializeAndListen() above.
    embedded_test_server()->StartAcceptingConnections();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(CrossSiteDocumentBlockingBaseTest);
};

// Most tests here use --site-per-process, which enables document blocking
// everywhere.
class CrossSiteDocumentBlockingTest : public CrossSiteDocumentBlockingBaseTest {
 public:
  CrossSiteDocumentBlockingTest() {}
  ~CrossSiteDocumentBlockingTest() override {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    IsolateAllSitesForTesting(command_line);
    CrossSiteDocumentBlockingBaseTest::SetUpCommandLine(command_line);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(CrossSiteDocumentBlockingTest);
};

IN_PROC_BROWSER_TEST_F(CrossSiteDocumentBlockingTest, BlockDocuments) {
  // Load a page that issues illegal cross-site document requests to bar.com.
  // The page uses XHR to request HTML/XML/JSON documents from bar.com, and
  // inspects if any of them were successfully received. This test is only
  // possible since we run the browser without the same origin policy.
  GURL foo_url("http://foo.com/cross_site_document_request.html");
  EXPECT_TRUE(NavigateToURL(shell(), foo_url));

  // The following are files under content/test/data/site_isolation. All
  // should be disallowed for cross site XHR under the document blocking policy.
  const char* blocked_resources[] = {
      "comment_valid.html", "html.txt",      "html4_dtd.html", "html4_dtd.txt",
      "html5_dtd.html",     "html5_dtd.txt", "json.txt",       "valid.html",
      "valid.json",         "valid.xml",     "xml.txt"};

  for (const char* resource : blocked_resources) {
    bool was_blocked;
    ASSERT_TRUE(ExecuteScriptAndExtractBool(
        shell(), base::StringPrintf("sendRequest(\"%s\");", resource),
        &was_blocked));
    EXPECT_TRUE(was_blocked);
  }

  // These files will be allowed for XHR under the document blocking policy once
  // sniffing is implemented, but they are blocked for now.
  const char* sniff_allowed_resources[] = {
      "js.html",  "comment_js.html", "js.xml",   "js.json", "js.txt",
      "img.html", "img.xml",         "img.json", "img.txt"};
  for (const char* resource : sniff_allowed_resources) {
    SCOPED_TRACE(base::StringPrintf("... while testing page: %s", resource));

    bool was_blocked;
    ASSERT_TRUE(ExecuteScriptAndExtractBool(
        shell(), base::StringPrintf("sendRequest(\"%s\");", resource),
        &was_blocked));
    // TODO(creis): This should be EXPECT_FALSE, once content sniffing is
    // implemented.
    EXPECT_TRUE(was_blocked);
  }

  // These files should be allowed for XHR under the document blocking policy.
  const char* allowed_resources[] = {"cors.html", "cors.xml", "cors.json",
                                     "cors.txt"};
  for (const char* resource : allowed_resources) {
    SCOPED_TRACE(base::StringPrintf("... while testing page: %s", resource));

    bool was_blocked;
    ASSERT_TRUE(ExecuteScriptAndExtractBool(
        shell(), base::StringPrintf("sendRequest(\"%s\");", resource),
        &was_blocked));
    EXPECT_FALSE(was_blocked);
  }
}

IN_PROC_BROWSER_TEST_F(CrossSiteDocumentBlockingTest, BlockForVariousTargets) {
  // This webpage loads a cross-site HTML page in different targets such as
  // <img>,<link>,<embed>, etc. Since the requested document is blocked, and one
  // character string (' ') is returned instead, this tests that the renderer
  // does not crash even when it receives a response body which is " ", whose
  // length is different from what's described in "content-length" for such
  // different targets.

  // TODO(nick): Split up these cases, and add positive assertions here about
  // what actually happens in these various resource-block cases.
  GURL foo("http://foo.com/cross_site_document_request_target.html");
  EXPECT_TRUE(NavigateToURL(shell(), foo));
  WaitForLoadStop(shell()->web_contents());
}

// Without any Site Isolation (in the base test class), there should be no
// document blocking.
IN_PROC_BROWSER_TEST_F(CrossSiteDocumentBlockingBaseTest,
                       DontBlockDocumentsByDefault) {
  if (AreAllSitesIsolatedForTesting())
    return;

  // Load a page that issues illegal cross-site document requests to bar.com.
  GURL foo_url("http://foo.com/cross_site_document_request.html");
  EXPECT_TRUE(NavigateToURL(shell(), foo_url));

  bool was_blocked;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
      shell(), "sendRequest(\"valid.html\");", &was_blocked));
  EXPECT_FALSE(was_blocked);
}

// Test class to verify that documents are blocked for isolated origins as well.
class CrossSiteDocumentBlockingIsolatedOriginTest
    : public CrossSiteDocumentBlockingBaseTest {
 public:
  CrossSiteDocumentBlockingIsolatedOriginTest() {}
  ~CrossSiteDocumentBlockingIsolatedOriginTest() override {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitchASCII(switches::kIsolateOrigins,
                                    "http://bar.com");
    CrossSiteDocumentBlockingBaseTest::SetUpCommandLine(command_line);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(CrossSiteDocumentBlockingIsolatedOriginTest);
};

IN_PROC_BROWSER_TEST_F(CrossSiteDocumentBlockingIsolatedOriginTest,
                       BlockDocumentsFromIsolatedOrigin) {
  if (AreAllSitesIsolatedForTesting())
    return;

  // Load a page that issues illegal cross-site document requests to the
  // isolated origin.
  GURL foo_url("http://foo.com/cross_site_document_request.html");
  EXPECT_TRUE(NavigateToURL(shell(), foo_url));

  bool was_blocked;
  ASSERT_TRUE(ExecuteScriptAndExtractBool(
      shell(), "sendRequest(\"valid.html\");", &was_blocked));
  EXPECT_TRUE(was_blocked);
}

}  // namespace content
