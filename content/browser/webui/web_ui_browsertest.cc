// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/browser/web_ui_controller_factory.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/shell/browser/shell.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

using WebUIBrowserTest = ContentBrowserTest;

static const char* kTestUrl = "chrome://content-webui-test/";

class TestWebUIController : public WebUIController {
 public:
  TestWebUIController(WebUI* web_ui) : WebUIController(web_ui) {
    std::unique_ptr<WebUIDataSource> source(
        WebUIDataSource::Create("content-webui-test"));
    // ???
    WebUIDataSource::Add(web_ui->GetWebContents()->GetBrowserContext(),
                         source.release());
  }

  bool OverrideHandleWebUIMessage(const GURL& source_url,
                                  const std::string& message,
                                  const base::ListValue& args) override {
    LOG(ERROR) << "TestWebUIController::OverrideHandleWebUIMessage"
               << "; source_url = " << source_url << "; message = " << message;
    return true;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TestWebUIController);
};

class TestWebUIControllerFactory : public WebUIControllerFactory {
 public:
  TestWebUIControllerFactory() {
    WebUIControllerFactory::RegisterFactory(this);
  }

  ~TestWebUIControllerFactory() override {
    WebUIControllerFactory::UnregisterFactoryForTesting(this);
  }

  WebUIController* CreateWebUIControllerForURL(WebUI* web_ui,
                                               const GURL& url) const override {
    if (url != GURL(kTestUrl))
      return nullptr;

    return new TestWebUIController(web_ui);
  }

  WebUI::TypeID GetWebUIType(BrowserContext* browser_context,
                             const GURL& url) const override {
    if (url != GURL(kTestUrl))
      return WebUI::kNoWebUI;

    return reinterpret_cast<WebUI::TypeID>(this);
  }

  bool UseWebUIForURL(BrowserContext* browser_context,
                      const GURL& url) const override {
    if (url != GURL(kTestUrl))
      return false;

    return true;
  }

  bool UseWebUIBindingsForURL(BrowserContext* browser_context,
                              const GURL& url) const override {
    if (url != GURL(kTestUrl))
      return false;

    return true;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(TestWebUIControllerFactory);
};

IN_PROC_BROWSER_TEST_F(WebUIBrowserTest, ViewCache) {
  ASSERT_TRUE(embedded_test_server()->Start());
  NavigateToURL(shell(), embedded_test_server()->GetURL("/simple_page.html"));
  int result = -1;
  NavigateToURL(shell(), GURL(kChromeUINetworkViewCacheURL));
  std::string script(
      "document.documentElement.innerHTML.indexOf('simple_page.html')");
  ASSERT_TRUE(ExecuteScriptAndExtractInt(
      shell()->web_contents(), "domAutomationController.send(" + script + ")",
      &result));
  ASSERT_NE(-1, result);
}

IN_PROC_BROWSER_TEST_F(WebUIBrowserTest, WebUISentFromUnloadHandler) {
  TestWebUIControllerFactory test_factory;
  GURL web_ui_url(kTestUrl);
  ASSERT_TRUE(embedded_test_server()->Start());
  GURL open_web_url(embedded_test_server()->GetURL("/title1.html"));

  // Navigate to the test Web UI.
  NavigateToURL(shell(), web_ui_url);
  EXPECT_EQ(web_ui_url, shell()->web_contents()->GetLastCommittedURL());
  int old_process_id =
      shell()->web_contents()->GetMainFrame()->GetProcess()->GetID();

  // Ask the unload handler to trigger a WebUI message.
  // TODO / DO NOT SUBMIT: Do this from an unload handler instead.
  std::string script = "chrome.send('blah')";
  EXPECT_TRUE(ExecuteScript(shell(), script));

  // Navigate to a non-Web-UI page.
  NavigateToURL(shell(), open_web_url);

  // Verify that the navigation caused a proces-swap.  The process swap is
  // 1) a desired security property (to isolate renderers hosting privileged
  //    WebUI pages from renderers hosting arbitrary pages from the open web).
  // 2) a prerequisite for the crash from https://crbug.com/780920.
  int new_process_id =
      shell()->web_contents()->GetMainFrame()->GetProcess()->GetID();
  EXPECT_NE(old_process_id, new_process_id);
  EXPECT_EQ(open_web_url, shell()->web_contents()->GetLastCommittedURL());

  // Implicitly verify that there was no browser-side crash (see
  // https://crbug.com/780920).
}

}  // namespace content
