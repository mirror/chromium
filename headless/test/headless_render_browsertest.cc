// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <functional>
#include <strstream>

#include "content/public/test/browser_test.h"
#include "headless/public/devtools/domains/dom_snapshot.h"
#include "headless/public/devtools/domains/page.h"
#include "headless/public/devtools/domains/runtime.h"
#include "headless/public/headless_devtools_client.h"
#include "headless/test/headless_render_test.h"
#include "net/test/embedded_test_server/http_response.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#define HEADLESS_RENDER_BROWSERTEST(clazz)                  \
  class HeadlessRenderBrowserTest##clazz : public clazz {}; \
  HEADLESS_ASYNC_DEVTOOLED_TEST_F(HeadlessRenderBrowserTest##clazz)

namespace headless {

namespace {

using dom_snapshot::GetSnapshotResult;
using dom_snapshot::DOMNode;
using dom_snapshot::LayoutTreeNode;

template <typename T, typename V>
std::vector<T> ElementsView(const std::vector<std::unique_ptr<V>>& elements,
                            std::function<bool(const V&)> filter,
                            std::function<T(const V&)> transform) {
  std::vector<T> result;
  for (const auto& element : elements) {
    if (filter(*element))
      result.push_back(transform(*element));
  }
  return result;
}

bool HasType(int type, const DOMNode& node) {
  return node.GetNodeType() == type;
}
bool HasName(const char* name, const DOMNode& node) {
  return node.GetNodeName() == name;
}
bool IsTag(const DOMNode& node) {
  return HasType(1, node);
}
bool IsText(const DOMNode& node) {
  return HasType(3, node);
}

std::vector<std::string> TextLayout(const GetSnapshotResult* snapshot) {
  return ElementsView<std::string, LayoutTreeNode>(
      *snapshot->GetLayoutTreeNodes(),
      [](const auto& node) { return node.HasLayoutText(); },
      [](const auto& node) { return node.GetLayoutText(); });
}

std::vector<const DOMNode*> FilterDOM(
    const GetSnapshotResult* snapshot,
    std::function<bool(const DOMNode&)> filter) {
  return ElementsView<const DOMNode*, DOMNode>(
      *snapshot->GetDomNodes(), filter, [](const auto& n) { return &n; });
}

std::vector<const DOMNode*> FindTags(const GetSnapshotResult* snapshot,
                                     const char* name = nullptr) {
  return FilterDOM(snapshot, [name](const auto& n) {
    return IsTag(n) && (!name || HasName(name, n));
  });
}

size_t IndexInDOM(const GetSnapshotResult* snapshot, const DOMNode* node) {
  for (size_t i = 0; i < snapshot->GetDomNodes()->size(); ++i) {
    if (snapshot->GetDomNodes()->at(i).get() == node)
      return i;
  }
  CHECK(false);
  return static_cast<size_t>(-1);
}

const DOMNode* GetAt(const GetSnapshotResult* snapshot, size_t index) {
  CHECK_LE(index, snapshot->GetDomNodes()->size());
  return snapshot->GetDomNodes()->at(index).get();
}

const DOMNode* NextNode(const GetSnapshotResult* snapshot,
                        const DOMNode* node) {
  return GetAt(snapshot, IndexInDOM(snapshot, node) + 1);
}

MATCHER_P(NodeName, expected, "") {
  return arg->GetNodeName() == expected;
}
MATCHER_P(NodeValue, expected, "") {
  return arg->GetNodeValue() == expected;
}
MATCHER_P(NodeType, expected, 0) {
  return arg->GetNodeType() == expected;
}

MATCHER_P(RemoteString, expected, "") {
  return arg->GetType() == runtime::RemoteObjectType::STRING &&
         arg->GetValue()->GetString() == expected;
}

MATCHER_P(RequestPath, expected, "") {
  return arg.relative_url == expected;
}

using testing::ElementsAre;
using testing::Eq;
using testing::StartsWith;
using net::test_server::HttpRequest;
using net::test_server::HttpResponse;
using net::test_server::BasicHttpResponse;
using net::test_server::RawHttpResponse;

}  // namespace

class HelloWorldTest : public HeadlessRenderTest {
 private:
  GURL GetPageUrl(HeadlessDevToolsClient* client) override {
    return GetURL("/hello.html");
  }

  void VerifyDom(GetSnapshotResult* dom_snapshot) override {
    EXPECT_THAT(FindTags(dom_snapshot),
                ElementsAre(NodeName("HTML"), NodeName("HEAD"),
                            NodeName("BODY"), NodeName("H1")));
    EXPECT_THAT(
        FilterDOM(dom_snapshot, IsText),
        ElementsAre(NodeValue("Hello headless world!"), NodeValue("\n")));
    EXPECT_THAT(TextLayout(dom_snapshot), ElementsAre("Hello headless world!"));
  }
};
HEADLESS_RENDER_BROWSERTEST(HelloWorldTest);

class TimeoutTest : public HeadlessRenderTest {
 private:
  void OnPageRenderCompleted() override {
    // Never complete.
  }

  GURL GetPageUrl(HeadlessDevToolsClient* client) override {
    return GetURL("/hello.html");
  }

  void VerifyDom(GetSnapshotResult* dom_snapshot) override {
    FAIL() << "Should not reach here";
  }

  void OnTimeout() override { SetTestCompleted(); }
};
HEADLESS_RENDER_BROWSERTEST(TimeoutTest);

class JavaScriptOverrideTitle_JsEnabled : public HeadlessRenderTest {
 private:
  std::unique_ptr<HttpResponse> OnResourceRequest(
      const HttpRequest& request) override {
    if (request.relative_url == "/foobar") {
      std::unique_ptr<BasicHttpResponse> http_response(new BasicHttpResponse);
      http_response->set_code(net::HttpStatusCode::HTTP_OK);
      http_response->set_content_type("text/html");
      http_response->set_content(
          "<html>\n"
          "<head>\n"
          "  <title>JavaScript is off</title>\n"
          "  <script language=\"JavaScript\">\n"
          "      <!-- Begin\n"
          "      document.title = 'JavaScript is on';\n"
          "      //  End -->\n"
          "  </script>\n"
          "</head>\n"
          "<body onload=\"settitle()\">\n"
          "  Hello, World!\n"
          "</body>\n"
          "</html>\n");
      return std::move(http_response);
    }
    return std::unique_ptr<net::test_server::HttpResponse>();
  }

  GURL GetPageUrl(HeadlessDevToolsClient* client) override {
    return GetURL("/foobar");
  }

  void VerifyDom(GetSnapshotResult* dom_snapshot) override {
    auto dom = FindTags(dom_snapshot, "TITLE");
    ASSERT_THAT(dom, ElementsAre(NodeName("TITLE")));
    const DOMNode* value = NextNode(dom_snapshot, dom[0]);
    EXPECT_THAT(value, NodeValue("JavaScript is on"));
  }
};
HEADLESS_RENDER_BROWSERTEST(JavaScriptOverrideTitle_JsEnabled);

class JavaScriptOverrideTitle_JsDisabled
    : public JavaScriptOverrideTitle_JsEnabled {
 private:
  void OverrideWebPreferences(WebPreferences* preferences) override {
    HeadlessRenderTest::OverrideWebPreferences(preferences);
    preferences->javascript_enabled = false;
  }

  void VerifyDom(GetSnapshotResult* dom_snapshot) override {
    auto dom = FindTags(dom_snapshot, "TITLE");
    ASSERT_THAT(dom, ElementsAre(NodeName("TITLE")));
    const DOMNode* value = NextNode(dom_snapshot, dom[0]);
    EXPECT_THAT(value, NodeValue("JavaScript is off"));
  }
};
HEADLESS_RENDER_BROWSERTEST(JavaScriptOverrideTitle_JsDisabled);

class JavaScriptConsoleErrors : public HeadlessRenderTest,
                                public runtime::ExperimentalObserver {
 private:
  HeadlessDevToolsClient* client_;
  std::vector<std::string> messages_;
  bool log_called_ = false;

  std::unique_ptr<HttpResponse> OnResourceRequest(
      const HttpRequest& request) override {
    if (request.relative_url == "/foobar") {
      std::unique_ptr<BasicHttpResponse> http_response(new BasicHttpResponse);
      http_response->set_code(net::HttpStatusCode::HTTP_OK);
      http_response->set_content_type("text/html");
      http_response->set_content(
          "<html>\n"
          "<head>\n"
          "  <script language=\"JavaScript\">\n"
          "    <![CDATA[\n"
          "     function image() {\n"
          "      window.open('<xsl:value-of select=\"/IMAGE/@href\" />');\n"
          "        }\n"
          "    ]]>\n"
          "  </script>\n"
          "</head>\n"
          "<body onload=\"func3()\">\n"
          "  <script type=\"text/javascript\">\n"
          "    func1()\n"
          "  </script>\n"
          "  <script type=\"text/javascript\">\n"
          "    func2()\n"
          "  </script>\n"
          "  <script type=\"text/javascript\">\n"
          "    console.log(\"Hello, Script!\");\n"
          "  </script>\n"
          "</body>\n"
          "</html>\n");
      return std::move(http_response);
    }
    return std::unique_ptr<net::test_server::HttpResponse>();
  }

  GURL GetPageUrl(HeadlessDevToolsClient* client) override {
    client_ = client;
    client_->GetRuntime()->GetExperimental()->AddObserver(this);
    base::RunLoop run_loop;
    client_->GetRuntime()->GetExperimental()->Enable(run_loop.QuitClosure());
    base::MessageLoop::ScopedNestableTaskAllower nest_loop(
        base::MessageLoop::current());
    run_loop.Run();
    return GetURL("/foobar");
  }

  void OnConsoleAPICalled(
      const runtime::ConsoleAPICalledParams& params) override {
    EXPECT_THAT(*params.GetArgs(), ElementsAre(RemoteString("Hello, Script!")));
    log_called_ = true;
  }

  void OnExceptionThrown(
      const runtime::ExceptionThrownParams& params) override {
    const runtime::ExceptionDetails* details = params.GetExceptionDetails();
    messages_.push_back(details->GetText() + " " +
                        details->GetException()->GetDescription());
  }

  void VerifyDom(GetSnapshotResult* dom_snapshot) override {
    client_->GetRuntime()->GetExperimental()->Disable();
    client_->GetRuntime()->GetExperimental()->RemoveObserver(this);
    EXPECT_TRUE(log_called_);
    EXPECT_THAT(messages_,
                ElementsAre(StartsWith("Uncaught SyntaxError:"),
                            StartsWith("Uncaught ReferenceError: func1"),
                            StartsWith("Uncaught ReferenceError: func2"),
                            StartsWith("Uncaught ReferenceError: func3")));
  }
};
HEADLESS_RENDER_BROWSERTEST(JavaScriptConsoleErrors);

class DelayedCompletion : public HeadlessRenderTest {
 private:
  base::TimeTicks start_;

  std::unique_ptr<HttpResponse> OnResourceRequest(
      const HttpRequest& request) override {
    if (request.relative_url == "/foobar") {
      std::unique_ptr<BasicHttpResponse> http_response(new BasicHttpResponse);
      http_response->set_code(net::HttpStatusCode::HTTP_OK);
      http_response->set_content_type("text/html");
      http_response->set_content(
          "<html>\n"
          "<body>\n"
          "  <script type=\"text/javascript\">\n"
          "    setTimeout(() => {\n"
          "      var div = document.getElementById('content');\n"
          "      var p = document.createElement('p');\n"
          "      p.textContent = 'delayed text';\n"
          "      div.appendChild(p);\n"
          "    }, 3000);\n"
          "  </script>\n"
          "  <div id=\"content\"/>\n"
          "</body>\n"
          "</html>\n");
      return std::move(http_response);
    }
    return std::unique_ptr<net::test_server::HttpResponse>();
  }

  GURL GetPageUrl(HeadlessDevToolsClient* client) override {
    start_ = base::TimeTicks::Now();
    return GetURL("/foobar");
  }

  void VerifyDom(GetSnapshotResult* dom_snapshot) override {
    base::TimeTicks end = base::TimeTicks::Now();
    EXPECT_THAT(
        FindTags(dom_snapshot),
        ElementsAre(NodeName("HTML"), NodeName("HEAD"), NodeName("BODY"),
                    NodeName("SCRIPT"), NodeName("DIV"), NodeName("P")));
    auto dom = FindTags(dom_snapshot, "P");
    ASSERT_THAT(dom, ElementsAre(NodeName("P")));
    const DOMNode* value = NextNode(dom_snapshot, dom[0]);
    EXPECT_THAT(value, NodeValue("delayed text"));
    // The page delays output for 3 seconds. Due to virtual time this should
    // take significantly less actual time.
    base::TimeDelta passed = end - start_;
    EXPECT_THAT(passed.InSecondsF(), testing::Le(2.9f));
  }
};
HEADLESS_RENDER_BROWSERTEST(DelayedCompletion);

class ClientRedirectChain : public HeadlessRenderTest {
 private:
  std::unique_ptr<HttpResponse> OnResourceRequest(
      const HttpRequest& request) override {
    GURL base = GetURL("/");
    if (request.relative_url == "/") {
      std::unique_ptr<BasicHttpResponse> http_response(new BasicHttpResponse);
      http_response->set_code(net::HttpStatusCode::HTTP_OK);
      http_response->set_content_type("text/html");
      http_response->set_content(
          "<html>\n"
          " <head>\n"
          "  <meta http-equiv=\"refresh\" "
          "    content=\"0; url=" +
          base.Resolve("/1").spec() +
          "\"/>\n"
          "  <title>Hello, World 0</title>\n"
          " </head>\n"
          " <body>http://www.example.com/</body>\n"
          "</html>\n");
      return std::move(http_response);
    } else if (request.relative_url == "/1") {
      std::unique_ptr<BasicHttpResponse> http_response(new BasicHttpResponse);
      http_response->set_code(net::HttpStatusCode::HTTP_OK);
      http_response->set_content_type("text/html");
      http_response->set_content(
          "<html>\n"
          " <head>\n"
          "  <title>Hello, World 1</title>\n"
          "  <script>\n"
          "   document.location='" +
          base.Resolve("/2").spec() +
          "';\n"
          "  </script>\n"
          " </head>\n"
          " <body>http://www.example.com/1</body>\n"
          "</html>\n");
      return std::move(http_response);
    } else if (request.relative_url == "/2") {
      std::unique_ptr<BasicHttpResponse> http_response(new BasicHttpResponse);
      http_response->set_code(net::HttpStatusCode::HTTP_OK);
      http_response->set_content_type("text/html");
      http_response->set_content(
          "<html>\n"
          " <head>\n"
          "  <title>Hello, World 2</title>\n"
          "  <script>\n"
          "    setTimeout("
          "      \"document.location='" +
          base.Resolve("/3").spec() +
          "'\","
          "      1000);\n"
          "  </script>\n"
          " </head>\n"
          " <body>http://www.example.com/2</body>\n"
          "</html>\n");
      return std::move(http_response);
    } else if (request.relative_url == "/3") {
      std::unique_ptr<BasicHttpResponse> http_response(new BasicHttpResponse);
      http_response->set_code(net::HttpStatusCode::HTTP_OK);
      http_response->set_content_type("text/html");
      http_response->set_content(
          "<html>\n"
          " <head>\n"
          "  <title>Pass</title>\n"
          " </head>\n"
          " <body>\n"
          "   http://www.example.com/3"
          "   <img src=\"pass\">\n"
          "</body>\n"
          "</html>\n");
      return std::move(http_response);
    }
    return std::unique_ptr<net::test_server::HttpResponse>();
  }

  GURL GetPageUrl(HeadlessDevToolsClient* client) override {
    return GetURL("/");
  }

  void VerifyDom(GetSnapshotResult* dom_snapshot) override {
    EXPECT_THAT(
        GetAccessLog(),
        ElementsAre(RequestPath("/"), RequestPath("/1"), RequestPath("/2"),
                    RequestPath("/3"), RequestPath("/pass")));
    auto dom = FindTags(dom_snapshot, "TITLE");
    ASSERT_THAT(dom, ElementsAre(NodeName("TITLE")));
    const DOMNode* value = NextNode(dom_snapshot, dom[0]);
    EXPECT_THAT(value, NodeValue("Pass"));
  }
};
HEADLESS_RENDER_BROWSERTEST(ClientRedirectChain);

}  // namespace headless
