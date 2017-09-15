// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "content/browser/frame_host/controllable_http_response.h"
#include "content/browser/frame_host/navigation_handle_impl.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/browser_side_navigation_policy.h"
#include "content/public/test/browser_test_utils.h"
#include "content/shell/browser/shell.h"

namespace content {

namespace {

class NavigationRecorder : public WebContentsObserver {
 public:
  NavigationRecorder(WebContents* web_contents)
      : WebContentsObserver(web_contents) {}

  // WebContentsObserver implementation.
  void DidStartNavigation(NavigationHandle* navigation_handle) override {
    did_start_navigation_urls_.push_back(navigation_handle->GetURL());
    WakeUp();
  }
  void ReadyToCommitNavigation(NavigationHandle* navigation_handle) override {
    ready_to_commit_navigation_urls_.push_back(navigation_handle->GetURL());
    WakeUp();
  }
  void DidFinishNavigation(NavigationHandle* navigation_handle) override {
    did_finish_navigation_urls_.push_back(navigation_handle->GetURL());
    WakeUp();
  }

  void WaitDidStartNavigation(unsigned int number) {
    while (did_start_navigation_urls_.size() < number)
      Wait();
  }
  void WaitReadyToCommitNavigation(unsigned int number) {
    while (ready_to_commit_navigation_urls_.size() < number)
      Wait();
  }
  void WaitDidFinishNavigation(unsigned int number) {
    while (did_finish_navigation_urls_.size() < number)
      Wait();
  }

  const std::vector<GURL>& did_start_navigation_urls() {
    return did_start_navigation_urls_;
  }
  const std::vector<GURL>& ready_to_commit_navigation_urls() {
    return ready_to_commit_navigation_urls_;
  }
  const std::vector<GURL>& did_finish_navigation_urls() {
    return did_finish_navigation_urls_;
  }

 private:
  void WakeUp() {
    if (loop_ && loop_->running())
      loop_->QuitClosure().Run();
  }
  void Wait() {
    loop_.reset(new base::RunLoop);
    loop_->Run();
    loop_.reset();
  }
  std::vector<GURL> did_start_navigation_urls_;
  std::vector<GURL> ready_to_commit_navigation_urls_;
  std::vector<GURL> did_finish_navigation_urls_;
  std::unique_ptr<base::RunLoop> loop_;
};

}  // namespace

// This reproduce a race condition issue:
// 1) A navigation to /page_1 starts, the headers are received, it gets
//    committed in the browser. The renderer waits for the response's body.
// 2) In the meantime, another navigation gets committed in the browser.
// 3) Before that the renderer gets notified of the new navigation, it finally
//    receive the response's body.
// 4) The browser gets notified of the commit of the first navigation instead
//    of the second one. The browser handles it as a new navigation (a third
//    one).
IN_PROC_BROWSER_TEST_F(ContentBrowserTest, example_use) {
  if (!IsBrowserSideNavigationEnabled())
    return;

  ControllableHttpResponse::Controller controller_1;
  ControllableHttpResponse::Controller controller_2;
  embedded_test_server()->RegisterRequestHandler(
      controller_1.HandleRequest("/page_1"));
  embedded_test_server()->RegisterRequestHandler(
      controller_2.HandleRequest("/page_2"));
  ASSERT_TRUE(embedded_test_server()->Start());

  // Start the test with an initial document.
  GURL main_url(embedded_test_server()->GetURL("/title1.html"));
  EXPECT_TRUE(NavigateToURL(shell(), main_url));
  EXPECT_TRUE(WaitForLoadStop(shell()->web_contents()));
  RenderFrameHostImpl* render_frame_host = static_cast<RenderFrameHostImpl*>(
      shell()->web_contents()->GetMainFrame());

  NavigationRecorder recorder(shell()->web_contents());
  GURL page_1(embedded_test_server()->GetURL("/page_1"));
  GURL page_2(embedded_test_server()->GetURL("/page_2"));

  // 1) Navigate to page_1. Send response's header but not its body.
  shell()->LoadURL(page_1);
  controller_1.WaitConnection();
  controller_1.Send(
      "HTTP/1.1 200 OK\r\n"
      "Connection: close\r\n"
      "Content-Length: 5\r\n"
      "Content-Type: text/html; charset=utf-8\r\n"
      "\r\n");
  recorder.WaitReadyToCommitNavigation(1);
  EXPECT_EQ(page_1, recorder.ready_to_commit_navigation_urls()[0]);
  EXPECT_EQ(page_1, render_frame_host->navigation_handle()->GetURL());

  // 2) Navigate to page_2. Send response's header but not its body.
  shell()->LoadURL(page_2);
  controller_2.WaitConnection();
  controller_2.Send(
      "HTTP/1.1 200 OK\r\n"
      "Connection: close\r\n"
      "Content-Length: 5\r\n"
      "Content-Type: text/html; charset=utf-8\r\n"
      "\r\n");
  // At the same time, send the response's body of page_1.
  // TODO(arthursonzogni): This test can still be potential flaky if
  // FrameMsg_CommitNavigation is received in the renderer before the response's
  // body is processed.
  controller_1.Send("hello");
  controller_1.Done();
  base::RunLoop().RunUntilIdle();
  recorder.WaitReadyToCommitNavigation(2);
  EXPECT_EQ(page_2, recorder.ready_to_commit_navigation_urls()[1]);
  // The first navigation gets replaced by the second one.
  recorder.WaitDidFinishNavigation(1);
  EXPECT_EQ(page_1, recorder.did_finish_navigation_urls()[0]);

  // A few time later, the first navigation comes back and replace the second
  // one.
  recorder.WaitDidFinishNavigation(3);
  EXPECT_EQ(page_1, recorder.did_finish_navigation_urls()[0]);
  EXPECT_EQ(page_2, recorder.did_finish_navigation_urls()[1]);
  EXPECT_EQ(page_1, recorder.did_finish_navigation_urls()[2]);
}

}  // namespace content
