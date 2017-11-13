// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "base/run_loop.h"
#include "components/viz/common/surfaces/surface_id.h"
#include "components/viz/common/surfaces/surface_sequence.h"
#include "content/browser/frame_host/frame_tree_node.h"
#include "content/browser/mus_util.h"
#include "content/browser/renderer_host/render_widget_host_view_child_frame.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/common/frame_messages.h"
#include "content/common/view_messages.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "content/shell/browser/shell.h"
#include "content/test/content_browser_test_utils_internal.h"
#include "content/test/test_content_browser_client.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "ui/gfx/geometry/size.h"

#if defined(USE_AURA)
#include "ui/aura/env.h"
#endif

namespace {

// Filters FrameHostMsg_UpdateFrameSinkId from an embedding renderer to its
// child frames. This allows the message to continue to the target child so that
// processing can be verified by tests.
class FrameSinkIdMessageFilter : public content::BrowserMessageFilter {
 public:
  FrameSinkIdMessageFilter();

  // Returns the new viz::FrameSinkId immediately if the IPC has been received.
  // Otherwise this will block the UI thread until it has been received, then it
  // will return the new viz::FrameSinkId.
  viz::FrameSinkId GetOrWaitForId();

 protected:
  ~FrameSinkIdMessageFilter() override {}

 private:
  void OnUpdateResizeParams(const gfx::Rect& frame_rect,
                            const content::ScreenInfo& screen_info,
                            uint64_t sequence_number,
                            const viz::SurfaceId& surface_id);
  void OnUpdateResizeParamsOnUI();

  bool OnMessageReceived(const IPC::Message& message) override;

  viz::FrameSinkId frame_sink_id_;

  base::RunLoop run_loop_;

  DISALLOW_COPY_AND_ASSIGN(FrameSinkIdMessageFilter);
};

FrameSinkIdMessageFilter::FrameSinkIdMessageFilter()
    : BrowserMessageFilter(FrameMsgStart) {}

viz::FrameSinkId FrameSinkIdMessageFilter::GetOrWaitForId() {
  // No-opt if already quit.
  run_loop_.Run();
  return frame_sink_id_;
}

void FrameSinkIdMessageFilter::OnUpdateResizeParams(
    const gfx::Rect& frame_rect,
    const content::ScreenInfo& screen_info,
    uint64_t sequence_number,
    const viz::SurfaceId& surface_id) {
  // Record the received value. We cannot check the current state of the child
  // frame, as it can only be processed on the UI thread, and we cannot block
  // here.
  frame_sink_id_ = surface_id.frame_sink_id();

  // There can be several updates before a valid viz::FrameSinkId is ready. Do
  // not quit |run_loop_| until after we receive a valid one.
  if (!frame_sink_id_.is_valid())
    return;

  // We can't nest on the IO thread. So tests will wait on the UI thread, so
  // post there to exit the nesting.
  content::BrowserThread::GetTaskRunnerForThread(content::BrowserThread::UI)
      ->PostTask(
          FROM_HERE,
          base::BindOnce(&FrameSinkIdMessageFilter::OnUpdateResizeParamsOnUI,
                         base::Unretained(this)));
}

void FrameSinkIdMessageFilter::OnUpdateResizeParamsOnUI() {
  run_loop_.Quit();
}

bool FrameSinkIdMessageFilter::OnMessageReceived(const IPC::Message& message) {
  IPC_BEGIN_MESSAGE_MAP(FrameSinkIdMessageFilter, message)
    IPC_MESSAGE_HANDLER(FrameHostMsg_UpdateResizeParams, OnUpdateResizeParams)
  IPC_END_MESSAGE_MAP()

  // We do not consume the message, so that we can verify the effects of it
  // being processed.
  return false;
}

}  // namespace

namespace content {

class RenderWidgetHostViewChildFrameTest : public ContentBrowserTest {
 public:
  RenderWidgetHostViewChildFrameTest() {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    IsolateAllSitesForTesting(command_line);
  }

  void SetUpOnMainThread() override {
    host_resolver()->AddRule("*", "127.0.0.1");
    SetupCrossSiteRedirector(embedded_test_server());
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  void CheckScreenWidth(RenderFrameHost* render_frame_host) {
    int width;
    ExecuteScriptAndGetValue(render_frame_host, "window.screen.width")
        ->GetAsInteger(&width);
    EXPECT_EQ(expected_screen_width_, width);
  }

  // Tests that the FrameSinkId of each child frame has been updated by the
  // RenderFrameProxy.
  void CheckFrameSinkId(RenderFrameHost* render_frame_host) {
    RenderWidgetHostViewBase* child_view =
        static_cast<RenderFrameHostImpl*>(render_frame_host)
            ->GetRenderWidgetHost()
            ->GetView();
    // Only interested in updated FrameSinkIds on child frames.
    if (!child_view || !child_view->IsRenderWidgetHostViewChildFrame())
      return;

    // Ensure that the received viz::FrameSinkId was correctly set on the child
    // frame.
    viz::FrameSinkId actual_frame_sink_id_ = child_view->GetFrameSinkId();
    EXPECT_EQ(expected_frame_sink_id_, actual_frame_sink_id_);

    // The viz::FrameSinkID will be replaced while the test blocks for
    // navigation. It should differ from the information stored in the child's
    // RenderWidgetHost.
    EXPECT_NE(base::checked_cast<uint32_t>(
                  child_view->GetRenderWidgetHost()->GetProcess()->GetID()),
              actual_frame_sink_id_.client_id());
    EXPECT_NE(base::checked_cast<uint32_t>(
                  child_view->GetRenderWidgetHost()->GetRoutingID()),
              actual_frame_sink_id_.sink_id());
  }

  void set_expected_frame_sink_id(viz::FrameSinkId frame_sink_id) {
    expected_frame_sink_id_ = frame_sink_id;
  }

  void set_expected_screen_width(int width) { expected_screen_width_ = width; }

 private:
  viz::FrameSinkId expected_frame_sink_id_;
  int expected_screen_width_;

  DISALLOW_COPY_AND_ASSIGN(RenderWidgetHostViewChildFrameTest);
};

// Tests that the screen is properly reflected for RWHVChildFrame.
IN_PROC_BROWSER_TEST_F(RenderWidgetHostViewChildFrameTest, Screen) {
  GURL main_url(embedded_test_server()->GetURL("/site_per_process_main.html"));
  NavigateToURL(shell(), main_url);

  FrameTreeNode* root = static_cast<WebContentsImpl*>(shell()->web_contents())
                            ->GetFrameTree()
                            ->root();

  // Load cross-site page into iframe.
  GURL cross_site_url(
      embedded_test_server()->GetURL("foo.com", "/title2.html"));
  NavigateFrameToURL(root->child_at(0), cross_site_url);

  int main_frame_screen_width = 0;
  ExecuteScriptAndGetValue(shell()->web_contents()->GetMainFrame(),
                           "window.screen.width")
      ->GetAsInteger(&main_frame_screen_width);
  set_expected_screen_width(main_frame_screen_width);
  EXPECT_FALSE(main_frame_screen_width == 0);

  shell()->web_contents()->ForEachFrame(
      base::Bind(&RenderWidgetHostViewChildFrameTest::CheckScreenWidth,
                 base::Unretained(this)));
}

// Test that auto-resize sizes in the top frame are propagated to OOPIF
// RenderWidgetHostViews. See https://crbug.com/726743.
IN_PROC_BROWSER_TEST_F(RenderWidgetHostViewChildFrameTest,
                       ChildFrameAutoResizeUpdate) {
  EXPECT_TRUE(NavigateToURL(
      shell(), embedded_test_server()->GetURL(
                   "a.com", "/cross_site_iframe_factory.html?a(b)")));

  FrameTreeNode* root = static_cast<WebContentsImpl*>(shell()->web_contents())
                            ->GetFrameTree()
                            ->root();
  root->current_frame_host()->render_view_host()->EnableAutoResize(
      gfx::Size(0, 0), gfx::Size(100, 100));

  RenderWidgetHostView* rwhv =
      root->child_at(0)->current_frame_host()->GetRenderWidgetHost()->GetView();

  // Fake an auto-resize update from the parent renderer.
  int routing_id =
      root->current_frame_host()->GetRenderWidgetHost()->GetRoutingID();
  ViewHostMsg_ResizeOrRepaint_ACK_Params params;
  params.view_size = gfx::Size(75, 75);
  params.flags = 0;
  root->current_frame_host()->GetRenderWidgetHost()->OnMessageReceived(
      ViewHostMsg_ResizeOrRepaint_ACK(routing_id, params));

  // RenderWidgetHostImpl has delayed auto-resize processing. Yield here to
  // let it complete.
  base::RunLoop run_loop;
  base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE,
                                                run_loop.QuitClosure());
  run_loop.Run();

  // The child frame's RenderWidgetHostView should now use the auto-resize value
  // for its visible viewport.
  EXPECT_EQ(gfx::Size(75, 75), rwhv->GetVisibleViewportSize());
}

// Tests that while in mus, the child frame receives an updated FrameSinkId
// representing the frame sink used by the RenderFrameProxy
IN_PROC_BROWSER_TEST_F(RenderWidgetHostViewChildFrameTest, ChildFrameSinkId) {
  // Only in mus do we expect a RenderFrameProxy to provide the FrameSinkId.
  if (!IsUsingMus())
    return;

  GURL main_url(embedded_test_server()->GetURL("/site_per_process_main.html"));
  NavigateToURL(shell(), main_url);

  FrameTreeNode* root = static_cast<WebContentsImpl*>(shell()->web_contents())
                            ->GetFrameTree()
                            ->root();
  scoped_refptr<FrameSinkIdMessageFilter> message_filter(
      new FrameSinkIdMessageFilter());
  root->current_frame_host()->GetProcess()->AddFilter(message_filter.get());

  // Load cross-site page into iframe.
  GURL cross_site_url(
      embedded_test_server()->GetURL("foo.com", "/title2.html"));
  // The child frame is created during this blocking call, on the UI thread.
  // This is racing the IPC we are testing for, which arrives on the IO thread.
  // Due to this we cannot get the pre-IPC value of the viz::FrameSinkId.
  NavigateFrameToURL(root->child_at(0), cross_site_url);

  // Ensure that the IPC providing the new viz::FrameSinkId. If it does not then
  // this test will timeout.
  set_expected_frame_sink_id(message_filter->GetOrWaitForId());

  shell()->web_contents()->ForEachFrame(
      base::Bind(&RenderWidgetHostViewChildFrameTest::CheckFrameSinkId,
                 base::Unretained(this)));
}

// A class to filter RequireSequence and SatisfySequence messages sent from
// an embedding renderer for its child's Surfaces.
class SurfaceRefMessageFilter : public BrowserMessageFilter {
 public:
  SurfaceRefMessageFilter()
      : BrowserMessageFilter(FrameMsgStart),
        require_message_loop_runner_(new content::MessageLoopRunner),
        satisfy_message_loop_runner_(new content::MessageLoopRunner),
        satisfy_received_(false),
        require_received_first_(false) {}

  void WaitForRequire() { require_message_loop_runner_->Run(); }

  void WaitForSatisfy() { satisfy_message_loop_runner_->Run(); }

  bool require_received_first() { return require_received_first_; }

 protected:
  ~SurfaceRefMessageFilter() override {}

 private:
  // BrowserMessageFilter:
  bool OnMessageReceived(const IPC::Message& message) override {
    IPC_BEGIN_MESSAGE_MAP(SurfaceRefMessageFilter, message)
      IPC_MESSAGE_HANDLER(FrameHostMsg_RequireSequence, OnRequire)
      IPC_MESSAGE_HANDLER(FrameHostMsg_SatisfySequence, OnSatisfy)
    IPC_END_MESSAGE_MAP()
    return false;
  }

  void OnRequire(const viz::SurfaceId& id,
                 const viz::SurfaceSequence sequence) {
    content::BrowserThread::PostTask(
        content::BrowserThread::UI, FROM_HERE,
        base::BindOnce(&SurfaceRefMessageFilter::OnRequireOnUI, this));
  }

  void OnRequireOnUI() {
    if (!satisfy_received_)
      require_received_first_ = true;
    require_message_loop_runner_->Quit();
  }

  void OnSatisfy(const viz::SurfaceSequence sequence) {
    content::BrowserThread::PostTask(
        content::BrowserThread::UI, FROM_HERE,
        base::BindOnce(&SurfaceRefMessageFilter::OnSatisfyOnUI, this));
  }

  void OnSatisfyOnUI() {
    satisfy_received_ = true;
    satisfy_message_loop_runner_->Quit();
  }

  scoped_refptr<content::MessageLoopRunner> require_message_loop_runner_;
  scoped_refptr<content::MessageLoopRunner> satisfy_message_loop_runner_;
  bool satisfy_received_;
  bool require_received_first_;

  DISALLOW_COPY_AND_ASSIGN(SurfaceRefMessageFilter);
};

// Test that when a child frame submits its first compositor frame, the
// embedding renderer process properly acquires and releases references to the
// new Surface. See https://crbug.com/701175.
// TODO(crbug.com/676384): Delete test with the rest of SurfaceSequence code.
IN_PROC_BROWSER_TEST_F(RenderWidgetHostViewChildFrameTest,
                       DISABLED_ChildFrameSurfaceReference) {
  EXPECT_TRUE(NavigateToURL(
      shell(), embedded_test_server()->GetURL(
                   "a.com", "/cross_site_iframe_factory.html?a(a)")));

  FrameTreeNode* root = static_cast<WebContentsImpl*>(shell()->web_contents())
                            ->GetFrameTree()
                            ->root();
  ASSERT_EQ(1U, root->child_count());

  scoped_refptr<SurfaceRefMessageFilter> filter = new SurfaceRefMessageFilter();
  root->current_frame_host()->GetProcess()->AddFilter(filter.get());

  GURL foo_url = embedded_test_server()->GetURL("foo.com", "/title1.html");
  NavigateFrameToURL(root->child_at(0), foo_url);

  // If one of these messages isn't received, this test times out.
  filter->WaitForRequire();
  filter->WaitForSatisfy();

  EXPECT_TRUE(filter->require_received_first());
}

}  // namespace content
