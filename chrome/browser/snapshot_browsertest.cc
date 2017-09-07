// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "build/build_config.h"

#include <stddef.h>

#include <algorithm>
#include <map>
#include <vector>

#include "chrome/test/base/in_process_browser_test.h"

#include "base/rand_util.h"
#include "base/strings/stringprintf.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
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
    "    var offset = 150;"
    "    function openNewWindow() {"
    "      window.open(\"/test\", \"\", "
    "          \"top=\"+offset+\",left=\"+offset+\",width=200,height=200\");"
    "      offset += 50;"
    "      window.domAutomationController.send(true);"
    "    }"
    "  </script>"
    "</body>";

class SnapshotBrowserTest : public InProcessBrowserTest {
 public:
  SnapshotBrowserTest() {}

  content::WebContents* GetWebContents(Browser* browser) {
    return browser->tab_strip_model()->GetActiveWebContents();
  }

  content::RenderViewHost* GetRenderViewHost(Browser* browser) {
    return GetWebContents(browser)->GetRenderViewHost();
  }

  content::RenderWidgetHost* GetRenderWidgetHost(Browser* browser) {
    return GetRenderViewHost(browser)->GetWidget();
  }

  content::RenderWidgetHostView* GetRenderWidgetHostView(Browser* browser) {
    return GetRenderWidgetHost(browser)->GetView();
  }

  void SetupTestServer() {
    // Use an embedded test server so we can open multiple windows in
    // the same renderer process, all referring to the same origin.
    embedded_test_server()->RegisterRequestHandler(base::Bind(
        &SnapshotBrowserTest::HandleRequest, base::Unretained(this)));
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

  struct ExpectedColor {
    ExpectedColor() : r(0), g(0), b(0) {}
    bool operator==(const ExpectedColor& other) const {
      return (r == other.r && g == other.g && b == other.b);
    }
    uint8_t r;
    uint8_t g;
    uint8_t b;
  };

  void PickRandomColor(ExpectedColor* expected) {
    expected->r = static_cast<uint8_t>(base::RandInt(0, 256));
    expected->g = static_cast<uint8_t>(base::RandInt(0, 256));
    expected->b = static_cast<uint8_t>(base::RandInt(0, 256));
  }

  struct SerialSnapshot {
    SerialSnapshot() : host_impl(nullptr) {}

    content::RenderWidgetHostImpl* host_impl;
    ExpectedColor color;
  };
  std::vector<SerialSnapshot> expected_snapshots_;

  void SyncSnapshotCallback(content::RenderWidgetHostImpl* rwhi,
                            const gfx::Image& image) {
    bool found = false;
    for (auto iter = expected_snapshots_.begin();
         iter != expected_snapshots_.end(); ++iter) {
      const SerialSnapshot& expected = *iter;
      if (expected.host_impl == rwhi) {
        found = true;

        const SkBitmap* bitmap = image.ToSkBitmap();
        SkColor color = bitmap->getColor(1, 1);

        EXPECT_EQ(static_cast<int>(SkColorGetR(color)),
                  static_cast<int>(expected.color.r))
            << "Red channels differed";
        EXPECT_EQ(static_cast<int>(SkColorGetG(color)),
                  static_cast<int>(expected.color.g))
            << "Green channels differed";
        EXPECT_EQ(static_cast<int>(SkColorGetB(color)),
                  static_cast<int>(expected.color.b))
            << "Blue channels differed";

        expected_snapshots_.erase(iter);
        break;
      }
    }
  }

  std::map<content::RenderWidgetHostImpl*, std::vector<ExpectedColor>>
      expected_async_snapshots_map_;

  void AsyncSnapshotCallback(content::RenderWidgetHostImpl* rwhi,
                             const gfx::Image& image) {
    auto iterator = expected_async_snapshots_map_.find(rwhi);
    ASSERT_NE(iterator, expected_async_snapshots_map_.end());
    std::vector<ExpectedColor>& expected_snapshots = iterator->second;
    const SkBitmap* bitmap = image.ToSkBitmap();
    SkColor color = bitmap->getColor(1, 1);
    bool found = false;
    // Find first instance of this color in the list and clear out all
    // of the entries up to that point. If it's not found, report
    // failure.
    for (auto iter = expected_snapshots.begin();
         iter != expected_snapshots.end(); ++iter) {
      const ExpectedColor& expected = *iter;
      if (SkColorGetR(color) == expected.r &&
          SkColorGetG(color) == expected.g &&
          SkColorGetB(color) == expected.b) {
        // Erase everything up to and including this color.
        expected_snapshots.erase(expected_snapshots.begin(), iter + 1);
        found = true;
        break;
      }
    }

    EXPECT_TRUE(found) << "Did not find color ("
                       << static_cast<int>(SkColorGetR(color)) << ", "
                       << static_cast<int>(SkColorGetG(color)) << ", "
                       << static_cast<int>(SkColorGetB(color))
                       << ") in expected snapshots for RWHI 0x" << (void*)rwhi;
  }
};

IN_PROC_BROWSER_TEST_F(SnapshotBrowserTest, SingleWindowTest) {
  // Use a smaller browser window to speed up the snapshots.
  browser()->window()->SetBounds(gfx::Rect(100, 100, 200, 200));

  SetupTestServer();

  content::RenderWidgetHostImpl* rwhi =
      static_cast<content::RenderWidgetHostImpl*>(
          GetRenderWidgetHost(browser()));

  for (int i = 0; i < 40; ++i) {
    SerialSnapshot expected;
    expected.host_impl = rwhi;
    PickRandomColor(&expected.color);

    std::string colorString = base::StringPrintf(
        "#%02x%02x%02x", expected.color.r, expected.color.g, expected.color.b);
    std::string script = std::string("fillWithColor(\"") + colorString + "\");";
    std::string result;
    EXPECT_TRUE(content::ExecuteScriptAndExtractString(
        GetRenderViewHost(browser()), script, &result));
    EXPECT_EQ(result, colorString);

    expected_snapshots_.push_back(expected);

    // Get the snapshot from the surface rather than the window. The
    // on-screen display path is verified by the GPU tests, and it
    // seems difficult to figure out the colorspace transformation
    // required to make these color comparisons.
    rwhi->GetSnapshotFromBrowser(
        base::Bind(&SnapshotBrowserTest::SyncSnapshotCallback,
                   base::Unretained(this), base::Unretained(rwhi)),
        true);
    while (expected_snapshots_.size() > 0) {
      base::RunLoop().RunUntilIdle();
    }
  }
}

IN_PROC_BROWSER_TEST_F(SnapshotBrowserTest, SyncMultiWindowTest) {
  // Use a smaller browser window to speed up the snapshots.
  browser()->window()->SetBounds(gfx::Rect(100, 100, 200, 200));

  SetupTestServer();

  for (int i = 0; i < 3; ++i) {
    bool result = false;
    EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
        GetRenderViewHost(browser()), "openNewWindow()", &result));
    EXPECT_TRUE(result);
  }

  base::RunLoop().RunUntilIdle();

  BrowserList* browser_list = BrowserList::GetInstance();
  EXPECT_EQ(4u, browser_list->size());

  for (int i = 0; i < 40; ++i) {
    for (int j = 0; j < 4; j++) {
      // Start each iteration by taking a snapshot with a different
      // browser instance.
      int browser_index = (i + j) % 4;
      Browser* browser = BrowserList::GetInstance()->get(browser_index);
      content::RenderWidgetHostImpl* rwhi =
          static_cast<content::RenderWidgetHostImpl*>(
              GetRenderWidgetHost(browser));

      SerialSnapshot expected;
      expected.host_impl = rwhi;
      PickRandomColor(&expected.color);

      std::string colorString =
          base::StringPrintf("#%02x%02x%02x", expected.color.r,
                             expected.color.g, expected.color.b);
      std::string script =
          std::string("fillWithColor(\"") + colorString + "\");";
      std::string result;
      EXPECT_TRUE(content::ExecuteScriptAndExtractString(
          GetRenderViewHost(browser), script, &result));
      EXPECT_EQ(result, colorString);
      expected_snapshots_.push_back(expected);
      // Get the snapshot from the surface rather than the window. The
      // on-screen display path is verified by the GPU tests, and it
      // seems difficult to figure out the colorspace transformation
      // required to make these color comparisons.
      rwhi->GetSnapshotFromBrowser(
          base::Bind(&SnapshotBrowserTest::SyncSnapshotCallback,
                     base::Unretained(this), base::Unretained(rwhi)),
          true);
    }

    while (expected_snapshots_.size() > 0) {
      base::RunLoop().RunUntilIdle();
    }
  }
}

IN_PROC_BROWSER_TEST_F(SnapshotBrowserTest, AsyncMultiWindowTest) {
  // Use a smaller browser window to speed up the snapshots.
  browser()->window()->SetBounds(gfx::Rect(100, 100, 200, 200));

  SetupTestServer();

  for (int i = 0; i < 3; ++i) {
    bool result = false;
    EXPECT_TRUE(content::ExecuteScriptAndExtractBool(
        GetRenderViewHost(browser()), "openNewWindow()", &result));
    EXPECT_TRUE(result);
  }

  base::RunLoop().RunUntilIdle();

  BrowserList* browser_list = BrowserList::GetInstance();
  EXPECT_EQ(4u, browser_list->size());

  for (int i = 0; i < 40; ++i) {
    for (int j = 0; j < 4; j++) {
      // Start each iteration by taking a snapshot with a different
      // browser instance.
      int browser_index = (i + j) % 4;
      Browser* browser = BrowserList::GetInstance()->get(browser_index);
      content::RenderWidgetHostImpl* rwhi =
          static_cast<content::RenderWidgetHostImpl*>(
              GetRenderWidgetHost(browser));

      std::vector<ExpectedColor>& expected_snapshots =
          expected_async_snapshots_map_[rwhi];

      // Pick a unique random color.
      ExpectedColor expected;
      do {
        PickRandomColor(&expected);
      } while (std::find(expected_snapshots.cbegin(), expected_snapshots.cend(),
                         expected) != expected_snapshots.cend());
      expected_snapshots.push_back(expected);

      std::string colorString = base::StringPrintf("#%02x%02x%02x", expected.r,
                                                   expected.g, expected.b);
      std::string script =
          std::string("fillWithColor(\"") + colorString + "\");";
      std::string result;
      EXPECT_TRUE(content::ExecuteScriptAndExtractString(
          GetRenderViewHost(browser), script, &result));
      EXPECT_EQ(result, colorString);
      // Get the snapshot from the surface rather than the window. The
      // on-screen display path is verified by the GPU tests, and it
      // seems difficult to figure out the colorspace transformation
      // required to make these color comparisons.
      rwhi->GetSnapshotFromBrowser(
          base::Bind(&SnapshotBrowserTest::AsyncSnapshotCallback,
                     base::Unretained(this), base::Unretained(rwhi)),
          true);
    }

    // Periodically yield and drain the async snapshot requests.
    if ((i % 2) == 0) {
      bool empty;
      do {
        empty = true;
        for (auto iter = expected_async_snapshots_map_.begin();
             iter != expected_async_snapshots_map_.end(); ++iter) {
          if (iter->second.size() > 0) {
            empty = false;
            break;
          }
        }
        if (!empty) {
          base::RunLoop().RunUntilIdle();
        }
      } while (!empty);
    }
  }
}

}  // anonymous namespace
