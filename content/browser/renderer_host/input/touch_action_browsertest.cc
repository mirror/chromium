// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/auto_reset.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "content/browser/renderer_host/input/synthetic_gesture.h"
#include "content/browser/renderer_host/input/synthetic_gesture_controller.h"
#include "content/browser/renderer_host/input/synthetic_gesture_target.h"
#include "content/browser/renderer_host/input/synthetic_smooth_scroll_gesture.h"
#include "content/browser/renderer_host/input/touch_event_queue.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/browser/renderer_host/render_widget_host_view_base.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/common/input/synthetic_gesture_params.h"
#include "content/common/input/synthetic_smooth_scroll_gesture_params.h"
#include "content/common/input_messages.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "content/shell/browser/shell.h"
#include "third_party/WebKit/public/platform/WebInputEvent.h"
#include "ui/events/event_switches.h"
#include "ui/latency/latency_info.h"

using blink::WebInputEvent;

namespace {

const char kTouchActionDataURL[] =
    "data:text/html;charset=utf-8,"
    "<!DOCTYPE html>"
    "<meta name='viewport' content='width=device-width'/>"
    "<style>"
    "html, body {"
    "  margin: 0;"
    "}"
    ".box {"
    "  height: 96px;"
    "  width: 96px;"
    "  border: 2px solid blue;"
    "}"
    ".spacer {"
    "  height: 10000px;"
    "  width: 10000px;"
    "}"
    ".ta-none { touch-action: none; }"
    ".ta-pany { touch-action: pan-y; }"
    "</style>"
    "<div class=box></div>"
    "<div class='box ta-none'></div>"
    "<div class='box ta-pany'></div>"
    "<div class=spacer></div>"
    "<script>"
    "  window.eventCounts = "
    "    {touchstart:0, touchmove:0, touchend: 0, touchcancel:0};"
    "  function countEvent(e) { eventCounts[e.type]++; }"
    "  for (var evt in eventCounts) { "
    "    document.addEventListener(evt, countEvent); "
    "  }"
    "  document.title='ready';"
    "</script>";

const char kTouchActionJankURL[] =
    "data:text/html;charset=utf-8,"
    "<!DOCTYPE html>"
    "<meta name='viewport' content='width=device-width'/>"
    "<style>"
    "html, body {"
    "  margin: 0;"
    "}"
    ".box {"
    "  height: 96px;"
    "  width: 96px;"
    "  border: 2px solid blue;"
    "}"
    ".position1 {"
    "  position: absolute;"
    "  top: 2px;"
    "  left: 2px;"
    "}"
    ".spacer {"
    "  height: 10000px;"
    "  width: 10000px;"
    "}"
    ".ta-pany { touch-action: pan-y; }"
    "</style>"
    "<div class='box position1 ta-pany'></div>"
    "<div class=spacer></div>"
    "<script>"
    "  document.title='ready';"
    "  document.addEventListener('DOMContentLoaded', function() {});"
    "  requestAnimationFrame(function() {"
    "    requestAnimationFrame(function() {"
    "      var jankAmount = 5000;"
    "      var end = (new Date()).getTime() + jankAmount;"
    "      while ((new Date()).getTime() < end) ;"
    "    });"
    "  });"
    "</script>";

}  // namespace

namespace content {


class TouchActionBrowserTest : public ContentBrowserTest {
 public:
  TouchActionBrowserTest() {}
  ~TouchActionBrowserTest() override {}

  RenderWidgetHostImpl* GetWidgetHost() {
    return RenderWidgetHostImpl::From(
        shell()->web_contents()->GetRenderViewHost()->GetWidget());
  }

  void OnSyntheticGestureCompleted(SyntheticGesture::Result result) {
    EXPECT_EQ(SyntheticGesture::GESTURE_FINISHED, result);
    runner_->Quit();
  }

 protected:
  void LoadURL(const char* touch_action_url) {
    const GURL data_url(touch_action_url);
    NavigateToURL(shell(), data_url);

    RenderWidgetHostImpl* host = GetWidgetHost();
    FrameWatcher frame_watcher(shell()->web_contents());
    host->GetView()->SetSize(gfx::Size(400, 400));

    base::string16 ready_title(base::ASCIIToUTF16("ready"));
    TitleWatcher watcher(shell()->web_contents(), ready_title);
    ignore_result(watcher.WaitAndGetTitle());

    // We need to wait until at least one frame has been composited
    // otherwise the injection of the synthetic gestures may get
    // dropped because of MainThread/Impl thread sync of touch event
    // regions.
    frame_watcher.WaitFrames(1);
  }

  // ContentBrowserTest:
  void SetUpCommandLine(base::CommandLine* cmd) override {
    cmd->AppendSwitchASCII(switches::kTouchEventFeatureDetection,
                           switches::kTouchEventFeatureDetectionEnabled);
    cmd->AppendSwitch(switches::kCompositorTouchAction);
  }

  int ExecuteScriptAndExtractInt(const std::string& script) {
    int value = 0;
    EXPECT_TRUE(content::ExecuteScriptAndExtractInt(
        shell(), "domAutomationController.send(" + script + ")", &value));
    return value;
  }

  int GetScrollTop() {
    return ExecuteScriptAndExtractInt("document.scrollingElement.scrollTop");
  }

  int GetScrollLeft() {
    return ExecuteScriptAndExtractInt("document.scrollingElement.scrollLeft");
  }

  base::Optional<cc::TouchAction> GetWhiteListedTouchAction() {
    return GetWidgetHost()->input_router()->WhiteListedTouchActionForTesting();
  }

  gfx::Vector2dF GetAccumulatedScrolling() {
    return GetWidgetHost()->input_router()->AccumulatedScrollingForTesting();
  }

  // Generate touch events for a synthetic scroll from |point| for |distance|.
  void DoTouchScroll(
      const gfx::Point& point,
      const gfx::Vector2d& distance,
      bool wait_until_scrolled,
      int expected_scroll_height,
      int expected_scroll_top,
      int expected_scroll_left,
      bool is_main_thread_janky = false,
      cc::TouchAction expected_white_listed_touch_action = cc::kTouchActionNone,
      gfx::Vector2dF expected_accumulated_scrolling = gfx::Vector2dF()) {
    EXPECT_EQ(0, GetScrollTop());

    int scrollHeight = ExecuteScriptAndExtractInt(
        "document.documentElement.scrollHeight");
    EXPECT_EQ(expected_scroll_height, scrollHeight);

    FrameWatcher frame_watcher(shell()->web_contents());

    SyntheticSmoothScrollGestureParams params;
    params.gesture_source_type = SyntheticGestureParams::TOUCH_INPUT;
    params.anchor = gfx::PointF(point);
    params.distances.push_back(-distance);

    runner_ = new MessageLoopRunner();

    std::unique_ptr<SyntheticSmoothScrollGesture> gesture(
        new SyntheticSmoothScrollGesture(params));
    GetWidgetHost()->QueueSyntheticGesture(
        std::move(gesture),
        base::BindOnce(&TouchActionBrowserTest::OnSyntheticGestureCompleted,
                       base::Unretained(this)));

    // Runs until we get the OnSyntheticGestureCompleted callback
    runner_->Run();
    runner_ = nullptr;

    if (is_main_thread_janky) {
      base::Optional<cc::TouchAction> white_listed_touch_action =
          GetWhiteListedTouchAction();
      EXPECT_TRUE(white_listed_touch_action.has_value());
      EXPECT_EQ(white_listed_touch_action.value(),
                expected_white_listed_touch_action);
      gfx::Vector2dF accumulated_scrolling = GetAccumulatedScrolling();
      if (expected_accumulated_scrolling.x() == 0) {
        EXPECT_EQ(accumulated_scrolling.x(),
                  expected_accumulated_scrolling.x());
      } else {
        EXPECT_LE(accumulated_scrolling.x(),
                  expected_accumulated_scrolling.x());
        EXPECT_LT(accumulated_scrolling.x(),
                  expected_accumulated_scrolling.x() / 2);
      }
      if (expected_accumulated_scrolling.y() == 0) {
        EXPECT_EQ(accumulated_scrolling.y(),
                  expected_accumulated_scrolling.y());
      } else {
        EXPECT_LE(accumulated_scrolling.y(),
                  expected_accumulated_scrolling.y());
        EXPECT_LT(accumulated_scrolling.y(),
                  expected_accumulated_scrolling.y() / 2);
      }
    }

    if (!is_main_thread_janky) {
      // Expect that the compositor scrolled at least one pixel while the
      // main thread was in a busy loop.
      while (wait_until_scrolled &&
             frame_watcher.LastMetadata().root_scroll_offset.y() <
                 expected_scroll_top &&
             frame_watcher.LastMetadata().root_scroll_offset.x() <
                 expected_scroll_left) {
        frame_watcher.WaitFrames(1);
      }

      // Check the scroll offset
      int scrollTop = GetScrollTop();
      int scrollLeft = GetScrollLeft();
      // Allow for 1px rounding inaccuracies for some screen sizes.
      if (expected_scroll_top != 0)
        EXPECT_LT(expected_scroll_top, scrollTop);
      else
        EXPECT_EQ(expected_scroll_top, scrollTop);
      if (expected_scroll_left != 0)
        EXPECT_LT(expected_scroll_left, scrollLeft);
      else
        EXPECT_EQ(expected_scroll_left, scrollLeft);
    }
  }

 private:
  scoped_refptr<MessageLoopRunner> runner_;

  DISALLOW_COPY_AND_ASSIGN(TouchActionBrowserTest);
};

// Mac doesn't yet have a gesture recognizer, so can't support turning touch
// events into scroll gestures.
// Will be fixed with http://crbug.com/337142
// Flaky on all platforms: https://crbug.com/376668
//
// Verify the test infrastructure works - we can touch-scroll the page and get a
// touchcancel as expected.
IN_PROC_BROWSER_TEST_F(TouchActionBrowserTest, DISABLED_DefaultAuto) {
  LoadURL(kTouchActionDataURL);

  int expected_scroll_top = 45 / 2;
  int expected_scroll_left = 0;
  DoTouchScroll(gfx::Point(50, 50), gfx::Vector2d(0, 45), true, 10300,
                expected_scroll_top, expected_scroll_left);

  EXPECT_EQ(1, ExecuteScriptAndExtractInt("eventCounts.touchstart"));
  EXPECT_GE(ExecuteScriptAndExtractInt("eventCounts.touchmove"), 1);
  EXPECT_EQ(1, ExecuteScriptAndExtractInt("eventCounts.touchend"));
  EXPECT_EQ(0, ExecuteScriptAndExtractInt("eventCounts.touchcancel"));
}

// Verify that touching a touch-action: none region disables scrolling and
// enables all touch events to be sent.
// Disabled on MacOS because it doesn't support touch input.
#if defined(OS_MACOSX)
#define MAYBE_TouchActionNone DISABLED_TouchActionNone
#else
#define MAYBE_TouchActionNone TouchActionNone
#endif
IN_PROC_BROWSER_TEST_F(TouchActionBrowserTest, MAYBE_TouchActionNone) {
  LoadURL(kTouchActionDataURL);

  int expected_scroll_top = 0;
  int expected_scroll_left = 0;
  DoTouchScroll(gfx::Point(50, 150), gfx::Vector2d(0, 45), false, 10300,
                expected_scroll_top, expected_scroll_left);

  EXPECT_EQ(1, ExecuteScriptAndExtractInt("eventCounts.touchstart"));
  EXPECT_GE(ExecuteScriptAndExtractInt("eventCounts.touchmove"), 1);
  EXPECT_EQ(1, ExecuteScriptAndExtractInt("eventCounts.touchend"));
  EXPECT_EQ(0, ExecuteScriptAndExtractInt("eventCounts.touchcancel"));
}

#if defined(OS_MACOSX)
#define MAYBE_PanYAllowed DISABLED_PanYAllowed
#else
#define MAYBE_PanYAllowed PanYAllowed
#endif
IN_PROC_BROWSER_TEST_F(TouchActionBrowserTest, MAYBE_PanYAllowed) {
  LoadURL(kTouchActionDataURL);

  int expected_scroll_top = 45 / 2;
  int expected_scroll_left = 0;
  DoTouchScroll(gfx::Point(50, 250), gfx::Vector2d(0, 45), false, 10300,
                expected_scroll_top, expected_scroll_left);
}

#if defined(OS_MACOSX)
#define MAYBE_PanXNotAllowed DISABLED_PanXNotAllowed
#else
#define MAYBE_PanXNotAllowed PanXNotAllowed
#endif
IN_PROC_BROWSER_TEST_F(TouchActionBrowserTest, MAYBE_PanXNotAllowed) {
  LoadURL(kTouchActionDataURL);

  int expected_scroll_top = 0;
  int expected_scroll_left = 0;
  DoTouchScroll(gfx::Point(50, 250), gfx::Vector2d(45, 0), false, 10300,
                expected_scroll_top, expected_scroll_left);
}

IN_PROC_BROWSER_TEST_F(TouchActionBrowserTest, PanYMainThreadJanky) {
  LoadURL(kTouchActionJankURL);

  DoTouchScroll(gfx::Point(50, 50), gfx::Vector2d(0, 45), false, 10000, 0, 0,
                true, cc::kTouchActionPanY, gfx::Vector2dF());
}

IN_PROC_BROWSER_TEST_F(TouchActionBrowserTest, PanXYMainThreadJanky) {
  LoadURL(kTouchActionJankURL);

  DoTouchScroll(gfx::Point(50, 50), gfx::Vector2d(45, 45), false, 10000, 0, 0,
                true, cc::kTouchActionPanY, gfx::Vector2dF(45, 0));
}

}  // namespace content
