// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "build/build_config.h"

#include <stddef.h>

#include "chrome/test/base/in_process_browser_test.h"

#include "base/rand_util.h"
#include "base/strings/stringprintf.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/test/browser_test_utils.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "ui/gfx/image/image.h"

namespace {

static const char kCanvasPageString[] =
    "<body>"
    "  <canvas id=\"canvas\" width=\"64\" height=\"64\""
    "    style=\"position:absolute;top:0px;left:0px;width:100%;"
    "    height=100%;margin:0;padding:0;\">"
    "  </canvas>"
    "  <script>"
    "    window.ctx = document.getElementById(\"canvas\").getContext(\"2d\");"
    "    function fillWithColor(color) {"
    "      ctx.fillStyle = color;"
    "      ctx.fillRect(0, 0, 64, 64);"
    "      window.domAutomationController.send(color);"
    "    }"
    "  </script>"
    "</body>";

class SnapshotBrowserTest : public InProcessBrowserTest {
 public:
  SnapshotBrowserTest() {}

  content::WebContents* GetWebContents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  content::RenderViewHost* GetRenderViewHost() {
    return GetWebContents()->GetRenderViewHost();
  }

  content::RenderWidgetHost* GetRenderWidgetHost() {
    return GetRenderViewHost()->GetWidget();
  }

  content::RenderWidgetHostView* GetRenderWidgetHostView() {
    return GetRenderWidgetHost()->GetView();
  }

  void SetupTestServer() {
    // Use an embedded test server so we can open multiple windows in
    // the same renderer process, all referring to the same origin.
    embedded_test_server()->RegisterRequestHandler(
        base::Bind(&SnapshotBrowserTest::HandleRequest,
                   base::Unretained(this)));
    ASSERT_TRUE(embedded_test_server()->Start());

    ASSERT_NO_FATAL_FAILURE(ui_test_utils::NavigateToURL(
        browser(), embedded_test_server()->GetURL("/test")));
  }

  std::unique_ptr<net::test_server::HttpResponse> HandleRequest(
      const net::test_server::HttpRequest& request) {
    GURL absolute_url = embedded_test_server()->GetURL(request.relative_url);
    if (absolute_url.path() != "/test")
      return std::unique_ptr<net::test_server::HttpResponse>();

    std::unique_ptr<net::test_server::BasicHttpResponse> http_response(
        new net::test_server::BasicHttpResponse());
    http_response->set_code(net::HTTP_OK);
    http_response->set_content(kCanvasPageString);
    http_response->set_content_type("text/html");
    return http_response;
  }

  bool got_last_image_ = false;
  gfx::Image last_image_;

  void SnapshotCallback(const gfx::Image& image) {
    last_image_ = image;
    got_last_image_ = true;
  }
};

IN_PROC_BROWSER_TEST_F(SnapshotBrowserTest, SingleWindowTest) {
  // Use a smaller browser window to speed up the snapshots.
  browser()->window()->SetBounds(gfx::Rect(100, 100, 200, 200));

  SetupTestServer();

  content::RenderWidgetHostImpl* rwhi =
      static_cast<content::RenderWidgetHostImpl*>(GetRenderWidgetHost());

  for (int i = 0; i < 40; ++i) {
    uint8_t channels[4];
    for (int j = 0; j < 3; ++j) {
      channels[j] = static_cast<uint8_t>(base::RandInt(0, 256));
    }
    channels[3] = 255u;

    std::string colorString = base::StringPrintf(
        "#%02x%02x%02x", channels[0], channels[1], channels[2]);
    std::string script = std::string("fillWithColor(\"") + colorString + "\");";
    std::string result;
    EXPECT_TRUE(content::ExecuteScriptAndExtractString(GetRenderViewHost(),
                                                       script, &result));
    EXPECT_EQ(result, colorString);

    got_last_image_ = false;
    // Get the snapshot from the surface rather than the window. The
    // on-screen display path is verified by the GPU tests, and it
    // seems difficult to figure out the colorspace transformation
    // required to make these color comparisons.
    rwhi->GetSnapshotFromBrowser(
        base::Bind(&SnapshotBrowserTest::SnapshotCallback,
                   base::Unretained(this)), true);
    while (!got_last_image_) {
      base::RunLoop().RunUntilIdle();
    }

    const SkBitmap* bitmap = last_image_.ToSkBitmap();
    SkColor color = bitmap->getColor(1, 1);

    EXPECT_EQ(static_cast<int>(SkColorGetR(color)),
              static_cast<int>(channels[0])) << "Red channels differed";
    EXPECT_EQ(static_cast<int>(SkColorGetG(color)),
              static_cast<int>(channels[1])) << "Green channels differed";
    EXPECT_EQ(static_cast<int>(SkColorGetB(color)),
              static_cast<int>(channels[2])) << "Blue channels differed";
  }
}

} // anonymous namespace
