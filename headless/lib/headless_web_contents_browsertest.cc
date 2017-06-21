// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string>
#include <vector>

#include "base/base64.h"
#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "content/public/common/isolated_world_ids.h"
#include "content/public/test/browser_test.h"
#include "headless/public/devtools/domains/dom_snapshot.h"
#include "headless/public/devtools/domains/page.h"
#include "headless/public/devtools/domains/runtime.h"
#include "headless/public/devtools/domains/security.h"
#include "headless/public/headless_browser.h"
#include "headless/public/headless_devtools_client.h"
#include "headless/public/headless_tab_socket.h"
#include "headless/public/headless_web_contents.h"
#include "headless/test/headless_browser_test.h"
#include "printing/features/features.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/geometry/size.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_BASIC_PRINTING)
#include "base/strings/string_number_conversions.h"
#include "pdf/pdf.h"
#include "printing/pdf_render_settings.h"
#include "printing/units.h"
#include "ui/gfx/geometry/rect.h"
#endif

using testing::ElementsAre;
using testing::UnorderedElementsAre;

namespace headless {

class HeadlessWebContentsTest : public HeadlessBrowserTest {};

IN_PROC_BROWSER_TEST_F(HeadlessWebContentsTest, Navigation) {
  EXPECT_TRUE(embedded_test_server()->Start());

  HeadlessBrowserContext* browser_context =
      browser()->CreateBrowserContextBuilder().Build();

  HeadlessWebContents* web_contents =
      browser_context->CreateWebContentsBuilder()
          .SetInitialURL(embedded_test_server()->GetURL("/hello.html"))
          .Build();
  EXPECT_TRUE(WaitForLoad(web_contents));

  EXPECT_THAT(browser_context->GetAllWebContents(),
              UnorderedElementsAre(web_contents));
}

IN_PROC_BROWSER_TEST_F(HeadlessWebContentsTest, WindowOpen) {
  EXPECT_TRUE(embedded_test_server()->Start());

  HeadlessBrowserContext* browser_context =
      browser()->CreateBrowserContextBuilder().Build();

  HeadlessWebContents* web_contents =
      browser_context->CreateWebContentsBuilder()
          .SetInitialURL(embedded_test_server()->GetURL("/window_open.html"))
          .Build();
  EXPECT_TRUE(WaitForLoad(web_contents));

  EXPECT_EQ(static_cast<size_t>(2),
            browser_context->GetAllWebContents().size());
}

IN_PROC_BROWSER_TEST_F(HeadlessWebContentsTest, Focus) {
  EXPECT_TRUE(embedded_test_server()->Start());

  HeadlessBrowserContext* browser_context =
      browser()->CreateBrowserContextBuilder().Build();

  HeadlessWebContents* web_contents =
      browser_context->CreateWebContentsBuilder()
          .SetInitialURL(embedded_test_server()->GetURL("/hello.html"))
          .Build();
  EXPECT_TRUE(WaitForLoad(web_contents));

  bool result;
  EXPECT_TRUE(EvaluateScript(web_contents, "document.hasFocus()")
                  ->GetResult()
                  ->GetValue()
                  ->GetAsBoolean(&result));
  EXPECT_TRUE(result);

  HeadlessWebContents* web_contents2 =
      browser_context->CreateWebContentsBuilder()
          .SetInitialURL(embedded_test_server()->GetURL("/hello.html"))
          .Build();
  EXPECT_TRUE(WaitForLoad(web_contents2));

  // Focus of different WebContents is independent.
  EXPECT_TRUE(EvaluateScript(web_contents, "document.hasFocus()")
                  ->GetResult()
                  ->GetValue()
                  ->GetAsBoolean(&result));
  EXPECT_TRUE(result);
  EXPECT_TRUE(EvaluateScript(web_contents2, "document.hasFocus()")
                  ->GetResult()
                  ->GetValue()
                  ->GetAsBoolean(&result));
  EXPECT_TRUE(result);
}

IN_PROC_BROWSER_TEST_F(HeadlessWebContentsTest, HandleSSLError) {
  net::EmbeddedTestServer https_server(net::EmbeddedTestServer::TYPE_HTTPS);
  https_server.SetSSLConfig(net::EmbeddedTestServer::CERT_EXPIRED);
  ASSERT_TRUE(https_server.Start());

  HeadlessBrowserContext* browser_context =
      browser()->CreateBrowserContextBuilder().Build();

  HeadlessWebContents* web_contents =
      browser_context->CreateWebContentsBuilder()
          .SetInitialURL(https_server.GetURL("/hello.html"))
          .Build();

  EXPECT_FALSE(WaitForLoad(web_contents));
}

namespace {
bool DecodePNG(std::string base64_data, SkBitmap* bitmap) {
  std::string png_data;
  if (!base::Base64Decode(base64_data, &png_data))
    return false;
  return gfx::PNGCodec::Decode(
      reinterpret_cast<unsigned const char*>(png_data.data()), png_data.size(),
      bitmap);
}
}  // namespace

// Parameter specifies whether --disable-gpu should be used.
class HeadlessWebContentsScreenshotTest
    : public HeadlessAsyncDevTooledBrowserTest,
      public ::testing::WithParamInterface<bool> {
 public:
  void SetUp() override {
    EnablePixelOutput();
    if (GetParam()) {
      UseSoftwareCompositing();
      SetUpWithoutGPU();
    } else {
      HeadlessAsyncDevTooledBrowserTest::SetUp();
    }
  }

  void RunDevTooledTest() override {
    std::unique_ptr<runtime::EvaluateParams> params =
        runtime::EvaluateParams::Builder()
            .SetExpression("document.body.style.background = '#0000ff'")
            .Build();
    devtools_client_->GetRuntime()->Evaluate(
        std::move(params),
        base::Bind(&HeadlessWebContentsScreenshotTest::OnPageSetupCompleted,
                   base::Unretained(this)));
  }

  void OnPageSetupCompleted(std::unique_ptr<runtime::EvaluateResult> result) {
    devtools_client_->GetPage()->GetExperimental()->CaptureScreenshot(
        page::CaptureScreenshotParams::Builder().Build(),
        base::Bind(&HeadlessWebContentsScreenshotTest::OnScreenshotCaptured,
                   base::Unretained(this)));
  }

  void OnScreenshotCaptured(
      std::unique_ptr<page::CaptureScreenshotResult> result) {
    std::string base64 = result->GetData();
    EXPECT_GT(base64.length(), 0U);
    SkBitmap result_bitmap;
    EXPECT_TRUE(DecodePNG(base64, &result_bitmap));

    EXPECT_EQ(800, result_bitmap.width());
    EXPECT_EQ(600, result_bitmap.height());
    SkColor actual_color = result_bitmap.getColor(400, 300);
    SkColor expected_color = SkColorSetRGB(0x00, 0x00, 0xff);
    EXPECT_EQ(expected_color, actual_color);
    FinishAsynchronousTest();
  }
};

HEADLESS_ASYNC_DEVTOOLED_TEST_P(HeadlessWebContentsScreenshotTest);

// Instantiate test case for both software and gpu compositing modes.
INSTANTIATE_TEST_CASE_P(HeadlessWebContentsScreenshotTests,
                        HeadlessWebContentsScreenshotTest,
                        ::testing::Bool());

#if BUILDFLAG(ENABLE_BASIC_PRINTING)
class HeadlessWebContentsPDFTest : public HeadlessAsyncDevTooledBrowserTest {
 public:
  const double kPaperWidth = 10;
  const double kPaperHeight = 15;
  const double kDocHeight = 50;
  // Number of color channels in a BGRA bitmap.
  const int kColorChannels = 4;
  const int kDpi = 300;

  void RunDevTooledTest() override {
    std::string height_expression = "document.body.style.height = '" +
                                    base::DoubleToString(kDocHeight) + "in'";
    std::unique_ptr<runtime::EvaluateParams> params =
        runtime::EvaluateParams::Builder()
            .SetExpression("document.body.style.background = '#123456';" +
                           height_expression)
            .Build();
    devtools_client_->GetRuntime()->Evaluate(
        std::move(params),
        base::Bind(&HeadlessWebContentsPDFTest::OnPageSetupCompleted,
                   base::Unretained(this)));
  }

  void OnPageSetupCompleted(std::unique_ptr<runtime::EvaluateResult> result) {
    devtools_client_->GetPage()->GetExperimental()->PrintToPDF(
        page::PrintToPDFParams::Builder()
            .SetPrintBackground(true)
            .SetPaperHeight(kPaperHeight)
            .SetPaperWidth(kPaperWidth)
            .SetMarginTop(0)
            .SetMarginBottom(0)
            .SetMarginLeft(0)
            .SetMarginRight(0)
            .Build(),
        base::Bind(&HeadlessWebContentsPDFTest::OnPDFCreated,
                   base::Unretained(this)));
  }

  void OnPDFCreated(std::unique_ptr<page::PrintToPDFResult> result) {
    std::string base64 = result->GetData();
    EXPECT_GT(base64.length(), 0U);
    std::string pdf_data;
    EXPECT_TRUE(base::Base64Decode(base64, &pdf_data));

    int num_pages;
    EXPECT_TRUE(chrome_pdf::GetPDFDocInfo(pdf_data.data(), pdf_data.size(),
                                          &num_pages, nullptr));
    EXPECT_EQ(std::ceil(kDocHeight / kPaperHeight), num_pages);

    for (int i = 0; i < num_pages; i++) {
      double width_in_points;
      double height_in_points;
      EXPECT_TRUE(chrome_pdf::GetPDFPageSizeByIndex(
          pdf_data.data(), pdf_data.size(), i, &width_in_points,
          &height_in_points));
      EXPECT_EQ(static_cast<int>(width_in_points),
                static_cast<int>(kPaperWidth * printing::kPointsPerInch));
      EXPECT_EQ(static_cast<int>(height_in_points),
                static_cast<int>(kPaperHeight * printing::kPointsPerInch));

      gfx::Rect rect(kPaperWidth * kDpi, kPaperHeight * kDpi);
      printing::PdfRenderSettings settings(
          rect, gfx::Point(0, 0), kDpi, true,
          printing::PdfRenderSettings::Mode::NORMAL);
      std::vector<uint8_t> page_bitmap_data(kColorChannels *
                                            settings.area.size().GetArea());
      EXPECT_TRUE(chrome_pdf::RenderPDFPageToBitmap(
          pdf_data.data(), pdf_data.size(), i, page_bitmap_data.data(),
          settings.area.size().width(), settings.area.size().height(),
          settings.dpi, settings.autorotate));
      EXPECT_EQ(0x56, page_bitmap_data[0]);  // B
      EXPECT_EQ(0x34, page_bitmap_data[1]);  // G
      EXPECT_EQ(0x12, page_bitmap_data[2]);  // R
    }
    FinishAsynchronousTest();
  }
};

HEADLESS_ASYNC_DEVTOOLED_TEST_F(HeadlessWebContentsPDFTest);
#endif

class HeadlessWebContentsSecurityTest
    : public HeadlessAsyncDevTooledBrowserTest,
      public security::ExperimentalObserver {
 public:
  void RunDevTooledTest() override {
    devtools_client_->GetSecurity()->GetExperimental()->AddObserver(this);
    devtools_client_->GetSecurity()->GetExperimental()->Enable(
        security::EnableParams::Builder().Build());
  }

  void OnSecurityStateChanged(
      const security::SecurityStateChangedParams& params) override {
    EXPECT_EQ(security::SecurityState::NEUTRAL, params.GetSecurityState());

    devtools_client_->GetSecurity()->GetExperimental()->Disable(
        security::DisableParams::Builder().Build());
    devtools_client_->GetSecurity()->GetExperimental()->RemoveObserver(this);
    FinishAsynchronousTest();
  }
};

HEADLESS_ASYNC_DEVTOOLED_TEST_F(HeadlessWebContentsSecurityTest);

class GetHeadlessTabSocketButNoTabSocket
    : public HeadlessAsyncDevTooledBrowserTest {
 public:
  void SetUp() override {
    options()->mojo_service_names.insert("headless::TabSocket");
    HeadlessAsyncDevTooledBrowserTest::SetUp();
  }

  void RunDevTooledTest() override {
    ASSERT_THAT(web_contents_->GetHeadlessTabSocket(), testing::IsNull());
    FinishAsynchronousTest();
  }

  bool GetAllowTabSockets() override { return false; }
};

HEADLESS_ASYNC_DEVTOOLED_TEST_F(GetHeadlessTabSocketButNoTabSocket);

class TabSocketTest : public HeadlessAsyncDevTooledBrowserTest,
                      public headless::HeadlessTabSocket::Listener,
                      public page::Observer,
                      public runtime::Observer {
 public:
  void SetUp() override {
    options()->mojo_service_names.insert("headless::TabSocket");
    HeadlessAsyncDevTooledBrowserTest::SetUp();
  }

  void RunDevTooledTest() override {
    // Note we must enable the page domain or the browser won't get told the
    // devtools frame ids.
    devtools_client_->GetPage()->AddObserver(this);
    devtools_client_->GetPage()->Enable();
    devtools_client_->GetRuntime()->AddObserver(this);
    devtools_client_->GetRuntime()->Enable();

    devtools_client_->GetPage()->GetExperimental()->GetResourceTree(
        page::GetResourceTreeParams::Builder().Build(),
        base::Bind(&TabSocketTest::OnResourceTree, base::Unretained(this)));
  }

  void OnResourceTree(std::unique_ptr<page::GetResourceTreeResult> result) {
    main_frame_id_ = result->GetFrameTree()->GetFrame()->GetId();
    RunTabSocketTest();
  }

  virtual void RunTabSocketTest() = 0;

  void OnExecutionContextCreated(
      const runtime::ExecutionContextCreatedParams& params) override {
    const base::DictionaryValue* dictionary;
    bool is_main_world;
    if (params.GetContext()->HasAuxData() &&
        params.GetContext()->GetAuxData()->GetAsDictionary(&dictionary) &&
        dictionary->GetBoolean("isDefault", &is_main_world) && !is_main_world) {
      isolated_world_name_to_context_id_[params.GetContext()->GetName()] =
          params.GetContext()->GetId();
    }
  }

  void ExpectJsException(std::unique_ptr<runtime::EvaluateResult> result) {
    EXPECT_TRUE(result->HasExceptionDetails());
    FinishAsynchronousTest();
  }

  void FailOnJsEvaluateException(
      std::unique_ptr<runtime::EvaluateResult> result) {
    if (!result->HasExceptionDetails())
      return;

    FinishAsynchronousTest();

    const runtime::ExceptionDetails* exception_details =
        result->GetExceptionDetails();
    FAIL() << exception_details->GetText()
           << (exception_details->HasException()
                   ? exception_details->GetException()->GetDescription().c_str()
                   : "");
  }

  bool GetAllowTabSockets() override { return true; }

  int GetIsolatedWorldContextId(const std::string& name) {
    const auto find_it = isolated_world_name_to_context_id_.find(name);
    if (find_it == isolated_world_name_to_context_id_.end()) {
      FinishAsynchronousTest();
      CHECK(false) << "Unknown isolated world " << name;
      return 0;
    }
    return find_it->second;
  }

  void CreateMainWorldTabSocket(base::Callback<void(bool)> callback) {
    web_contents_->InstallHeadlessTabSocketBindings(
        main_frame_id_, content::ISOLATED_WORLD_ID_GLOBAL, callback);
  }

  void CreateIsolatedWorldTabSocket(std::string isolated_world_name,
                                    std::string devtools_frame_id,
                                    base::Callback<void(int)> callback) {
    devtools_client_->GetPage()->GetExperimental()->CreateIsolatedWorld(
        page::CreateIsolatedWorldParams::Builder()
            .SetFrameId(devtools_frame_id)
            .SetWorldName(isolated_world_name)
            .Build(),
        base::Bind(&TabSocketTest::OnCreatedIsolatedWorld,
                   base::Unretained(this), devtools_frame_id, callback));
  }

 protected:
  std::string main_frame_id_;

 private:
  void OnCreatedIsolatedWorld(
      std::string devtools_frame_id,
      base::Callback<void(int)> callback,
      std::unique_ptr<page::CreateIsolatedWorldResult> result) {
    web_contents_->InstallHeadlessTabSocketBindings(
        devtools_frame_id, result->GetWorldId(),
        base::Bind(&TabSocketTest::OnInstalledHeadlessTabSocket,
                   base::Unretained(this), callback, result->GetWorldId()));
  }

  void OnInstalledHeadlessTabSocket(base::Callback<void(int)> callback,
                                    int world_id,
                                    bool success) {
    EXPECT_TRUE(success);
    if (!success) {
      FinishAsynchronousTest();
    } else {
      callback.Run(world_id);
    }
  }

  std::map<std::string, int> isolated_world_name_to_context_id_;
};

class MainWorldHeadlessTabSocketTest : public TabSocketTest {
 public:
  void RunTabSocketTest() override {
    CreateMainWorldTabSocket(base::Bind(
        &MainWorldHeadlessTabSocketTest::OnInstalledHeadlessTabSocket,
        base::Unretained(this)));
  }

  void OnInstalledHeadlessTabSocket(bool success) {
    ASSERT_TRUE(success);
    devtools_client_->GetRuntime()->Evaluate(
        R"(window.TabSocket.onmessage =
            function(message) {
              window.TabSocket.send('Embedder sent us: ' + message);
            };
          )",
        base::Bind(&MainWorldHeadlessTabSocketTest::FailOnJsEvaluateException,
                   base::Unretained(this)));

    HeadlessTabSocket* headless_tab_socket =
        web_contents_->GetHeadlessTabSocket();
    DCHECK(headless_tab_socket);

    headless_tab_socket->SendMessageToTab("One", main_frame_id_,
                                          content::ISOLATED_WORLD_ID_GLOBAL);
    headless_tab_socket->SendMessageToTab("Two", main_frame_id_,
                                          content::ISOLATED_WORLD_ID_GLOBAL);
    headless_tab_socket->SendMessageToTab("Three", main_frame_id_,
                                          content::ISOLATED_WORLD_ID_GLOBAL);
    headless_tab_socket->SetListener(this);
  }

  void OnMessageFromTab(const std::string& message,
                        const std::string& devtools_frame_id,
                        int world_id) override {
    EXPECT_EQ(0, world_id);
    messages_.push_back(message);
    if (messages_.size() == 3u) {
      EXPECT_THAT(messages_,
                  ElementsAre("Embedder sent us: One", "Embedder sent us: Two",
                              "Embedder sent us: Three"));
      FinishAsynchronousTest();
    }
  }

 private:
  std::vector<std::string> messages_;
};

HEADLESS_ASYNC_DEVTOOLED_TEST_F(MainWorldHeadlessTabSocketTest);

class MainWorldHeadlessTabSocketBindingsNotInstalledTest
    : public TabSocketTest {
 public:
  void RunTabSocketTest() override {
    CreateIsolatedWorldTabSocket(
        "Test World", main_frame_id_,
        base::Bind(&MainWorldHeadlessTabSocketBindingsNotInstalledTest::
                       OnIsolatedWorldCreated,
                   base::Unretained(this)));
  }

  void OnIsolatedWorldCreated(int world_id) {
    // We expect this to fail because TabSocket bindings where injected into the
    // isolated world not the main world.
    devtools_client_->GetRuntime()->Evaluate(
        "window.TabSocket.send('This should not work!');",
        base::Bind(&MainWorldHeadlessTabSocketBindingsNotInstalledTest::
                       ExpectJsException,
                   base::Unretained(this)));

    HeadlessTabSocket* headless_tab_socket =
        web_contents_->GetHeadlessTabSocket();
    DCHECK(headless_tab_socket);

    headless_tab_socket->SetListener(this);
  }

  void OnMessageFromTab(const std::string&, const std::string&, int) override {
#ifdef OS_WIN
    FinishAsynchronousTest();
    FAIL() << "Should not receive a message from the tab!";
#else
    FAIL() << "Should not receive a message from the tab!";
    FinishAsynchronousTest();
#endif
  }
};

HEADLESS_ASYNC_DEVTOOLED_TEST_F(
    MainWorldHeadlessTabSocketBindingsNotInstalledTest);

class IsolatedWorldHeadlessTabSocketTest : public TabSocketTest {
 public:
  void RunTabSocketTest() override {
    CreateIsolatedWorldTabSocket(
        "Test World", main_frame_id_,
        base::Bind(&IsolatedWorldHeadlessTabSocketTest::OnIsolatedWorldCreated,
                   base::Unretained(this)));
  }

  void OnIsolatedWorldCreated(int world_id) {
    HeadlessTabSocket* headless_tab_socket =
        web_contents_->GetHeadlessTabSocket();
    DCHECK(headless_tab_socket);
    headless_tab_socket->SendMessageToTab("Hello!!!", main_frame_id_, world_id);
    headless_tab_socket->SetListener(this);

    devtools_client_->GetRuntime()->Evaluate(
        runtime::EvaluateParams::Builder()
            .SetExpression(
                R"(window.TabSocket.onmessage =
                    function(message) {
                      TabSocket.send('Embedder sent us: ' + message);
                    };
                  )")
            .SetContextId(GetIsolatedWorldContextId("Test World"))
            .Build(),
        base::Bind(
            &IsolatedWorldHeadlessTabSocketTest::FailOnJsEvaluateException,
            base::Unretained(this)));

    isolated_world_id_ = world_id;
  }

  void OnMessageFromTab(const std::string& message,
                        const std::string& devtools_frame_id,
                        int world_id) override {
    EXPECT_EQ("Embedder sent us: Hello!!!", message);
    EXPECT_EQ(*isolated_world_id_, world_id);
    FinishAsynchronousTest();
  }

  base::Optional<int> isolated_world_id_;
};

HEADLESS_ASYNC_DEVTOOLED_TEST_F(IsolatedWorldHeadlessTabSocketTest);

class IsolatedWorldHeadlessTabSocketBindingsNotInstalled
    : public TabSocketTest {
 public:
  void RunTabSocketTest() override {
    devtools_client_->GetPage()->GetExperimental()->CreateIsolatedWorld(
        page::CreateIsolatedWorldParams::Builder()
            .SetFrameId(main_frame_id_)
            .Build());
  }

  void OnExecutionContextCreated(
      const runtime::ExecutionContextCreatedParams& params) override {
    const base::DictionaryValue* dictionary;
    std::string frame_id;
    bool is_main_world;
    // If the isolated world was created then eval some script in it.
    if (params.GetContext()->HasAuxData() &&
        params.GetContext()->GetAuxData()->GetAsDictionary(&dictionary) &&
        dictionary->GetString("frameId", &frame_id) &&
        frame_id == main_frame_id_ &&
        dictionary->GetBoolean("isDefault", &is_main_world) && !is_main_world) {
      // We expect this to fail because the TabSocket is being injected into the
      // main world.
      devtools_client_->GetRuntime()->Evaluate(
          runtime::EvaluateParams::Builder()
              .SetExpression("window.TabSocket.send('This should not work!');")
              .SetContextId(params.GetContext()->GetId())
              .Build(),
          base::Bind(&IsolatedWorldHeadlessTabSocketBindingsNotInstalled::
                         ExpectJsException,
                     base::Unretained(this)));
    }
  }

  void OnMessageFromTab(const std::string&, const std::string&, int) override {
#ifdef OS_WIN
    FinishAsynchronousTest();
    FAIL() << "Should not receive a message from the tab!";
#else
    FAIL() << "Should not receive a message from the tab!";
    FinishAsynchronousTest();
#endif
  }
};

HEADLESS_ASYNC_DEVTOOLED_TEST_F(
    IsolatedWorldHeadlessTabSocketBindingsNotInstalled);

class MultipleIframesIsolatedWorldHeadlessTabSocketTest : public TabSocketTest {
 public:
  void RunTabSocketTest() override {
    EXPECT_TRUE(embedded_test_server()->Start());
    devtools_client_->GetPage()->Navigate(
        embedded_test_server()->GetURL("/two_iframes.html").spec());
  }

  void OnLoadEventFired(const page::LoadEventFiredParams& params) override {
    devtools_client_->GetPage()->Disable();
    devtools_client_->GetPage()->RemoveObserver(this);
    devtools_client_->GetDOMSnapshot()->GetExperimental()->GetSnapshot(
        dom_snapshot::GetSnapshotParams::Builder()
            .SetComputedStyleWhitelist(std::vector<std::string>())
            .Build(),
        base::Bind(
            &MultipleIframesIsolatedWorldHeadlessTabSocketTest::OnSnapshot,
            base::Unretained(this)));
  }

  void OnSnapshot(std::unique_ptr<dom_snapshot::GetSnapshotResult> result) {
    bool seen_main_frame = false;
    for (const auto& node : *result->GetDomNodes()) {
      if (node->HasFrameId()) {
        std::string frame_name;
        if (node->GetNodeName() == "IFRAME") {
          // Use the iframe id attribute for the name.
          for (const auto& key_value : *node->GetAttributes()) {
            if (key_value->GetName() == "id") {
              frame_name = key_value->GetValue();
            }
          }
          CHECK(!frame_name.empty());
        } else {
          if (seen_main_frame)
            continue;
          seen_main_frame = true;
          frame_name = "main frame";
        }
        CreateIsolatedWorldTabSocket(
            frame_name, node->GetFrameId(),
            base::Bind(&MultipleIframesIsolatedWorldHeadlessTabSocketTest::
                           OnIsolatedWorldCreated,
                       base::Unretained(this), frame_name, node->GetFrameId()));
      }
    }
  }

  void OnIsolatedWorldCreated(std::string frame_name,
                              std::string devtools_frame_id,
                              int world_id) {
    HeadlessTabSocket* headless_tab_socket =
        web_contents_->GetHeadlessTabSocket();
    DCHECK(headless_tab_socket);
    headless_tab_socket->SendMessageToTab("Hello!!!", devtools_frame_id,
                                          world_id);
    headless_tab_socket->SetListener(this);

    devtools_client_->GetRuntime()->Evaluate(
        runtime::EvaluateParams::Builder()
            .SetExpression(base::StringPrintf(
                R"(window.TabSocket.onmessage =
                    function(message) {
                      TabSocket.send('Echo from %s: ' + message);
                    };
                  )",
                frame_name.c_str()))
            .SetContextId(GetIsolatedWorldContextId(frame_name))
            .Build(),
        base::Bind(&MultipleIframesIsolatedWorldHeadlessTabSocketTest::
                       FailOnJsEvaluateException,
                   base::Unretained(this)));
  }

  void OnMessageFromTab(const std::string& message,
                        const std::string& devtools_frame_id,
                        int world_id) override {
    messages_.push_back(message);
    if (messages_.size() < 3)
      return;
    EXPECT_THAT(messages_,
                UnorderedElementsAre("Echo from main frame: Hello!!!",
                                     "Echo from iframe1: Hello!!!",
                                     "Echo from iframe2: Hello!!!"));
    FinishAsynchronousTest();
  }

  std::vector<std::string> messages_;
};

HEADLESS_ASYNC_DEVTOOLED_TEST_F(
    MultipleIframesIsolatedWorldHeadlessTabSocketTest);

class SingleTabMultipleIsolatedWorldsHeadlessTabSocketTest
    : public TabSocketTest {
 public:
  void RunTabSocketTest() override {
    CreateIsolatedWorldTabSocket(
        "Isolated World 1", main_frame_id_,
        base::Bind(&SingleTabMultipleIsolatedWorldsHeadlessTabSocketTest::
                       OnIsolatedWorldCreated,
                   base::Unretained(this), "Isolated World 1"));

    CreateIsolatedWorldTabSocket(
        "Isolated World 2", main_frame_id_,
        base::Bind(&SingleTabMultipleIsolatedWorldsHeadlessTabSocketTest::
                       OnIsolatedWorldCreated,
                   base::Unretained(this), "Isolated World 2"));

    CreateIsolatedWorldTabSocket(
        "Isolated World 3", main_frame_id_,
        base::Bind(&SingleTabMultipleIsolatedWorldsHeadlessTabSocketTest::
                       OnIsolatedWorldCreated,
                   base::Unretained(this), "Isolated World 3"));
  }

  void OnIsolatedWorldCreated(std::string frame_name, int world_id) {
    HeadlessTabSocket* headless_tab_socket =
        web_contents_->GetHeadlessTabSocket();
    DCHECK(headless_tab_socket);
    headless_tab_socket->SendMessageToTab("Hello!!!", main_frame_id_, world_id);
    headless_tab_socket->SetListener(this);

    devtools_client_->GetRuntime()->Evaluate(
        runtime::EvaluateParams::Builder()
            .SetExpression(base::StringPrintf(
                R"(window.TabSocket.onmessage =
                    function(message) {
                      TabSocket.send('Echo from %s: ' + message);
                    };
                  )",
                frame_name.c_str()))
            .SetContextId(GetIsolatedWorldContextId(frame_name))
            .Build(),
        base::Bind(&SingleTabMultipleIsolatedWorldsHeadlessTabSocketTest::
                       FailOnJsEvaluateException,
                   base::Unretained(this)));
  }

  void OnMessageFromTab(const std::string& message,
                        const std::string& devtools_frame_id,
                        int world_id) override {
    messages_.push_back(message);
    if (messages_.size() < 3)
      return;
    EXPECT_THAT(messages_,
                UnorderedElementsAre("Echo from Isolated World 1: Hello!!!",
                                     "Echo from Isolated World 2: Hello!!!",
                                     "Echo from Isolated World 3: Hello!!!"));
    FinishAsynchronousTest();
  }

  std::vector<std::string> messages_;
};

HEADLESS_ASYNC_DEVTOOLED_TEST_F(
    SingleTabMultipleIsolatedWorldsHeadlessTabSocketTest);

}  // namespace headless
