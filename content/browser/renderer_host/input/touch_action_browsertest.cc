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
#include "content/browser/renderer_host/input/synthetic_pinch_gesture.h"
#include "content/browser/renderer_host/input/synthetic_smooth_scroll_gesture.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/browser/renderer_host/render_widget_host_view_base.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/common/input/synthetic_gesture_params.h"
#include "content/common/input/synthetic_pinch_gesture_params.h"
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
    ".spacer { height: 10000px; }"
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
    "  box-sizing: border-box;"
    "  height: 100px;"
    "  width: 100px;"
    "  border: 2px solid blue;"
    "}"
    ".spacer {"
    "  height: 10000px;"
    "  width: 10000px;"
    "}"
    ".ta-auto {"
    "  position: absolute;"
    "  top: 52px;"
    "  left: 52px;"
    "  will-change: transform;"
    "  touch-action: auto;"
    "}"
    ".ta-pany {"
    "  position: absolute;"
    "  top: 102px;"
    "  left: 2px;"
    "  will-change: transform;"
    "  touch-action: pan-y;"
    "}"
    ".ta-panx {"
    "  position: absolute;"
    "  top: 2px;"
    "  left: 102px;"
    "  will-change: transform;"
    "  touch-action: pan-x;"
    "}"
    "</style>"
    "<div class='box ta-panx'></div>"
    "<div class='box ta-pany'></div>"
    "<div class='box ta-auto'></div>"
    "<div class=spacer></div>"
    "<script>"
    "  document.title='ready';"
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

    load_url_called_ = true;
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

  void JankMainThread() {
    std::string script =
        "var end = (new Date()).getTime() + 200; while ((new Date()).getTime() "
        "< end) ;";
    // content::ExecuteScriptAsync sends the script to the renderer through IPC
    // and do not wait for response. This is what we need here because we want
    // the main thread to pause but the browser keeps functioning.
    content::ExecuteScriptAsync(shell(), script);
  }

  // Must call LoadURL() before calling this function
  float GetPageScaleFactor() {
    FrameWatcher frame_watcher(shell()->web_contents());
    float page_scale_factor = frame_watcher.LastMetadata().page_scale_factor;
    // Default value (0) actually indicates that the value is 1.
    return page_scale_factor == 0 ? 1 : page_scale_factor;
  }

  int GetScrollTop() {
    return ExecuteScriptAndExtractInt("document.scrollingElement.scrollTop");
  }

  int GetScrollLeft() {
    return ExecuteScriptAndExtractInt("document.scrollingElement.scrollLeft");
  }

  // Generate touch events for a synthetic scroll from |point| for |distance|.
  void DoTouchScroll(const gfx::Point& point,
                     const gfx::Vector2d& distance,
                     bool wait_until_scrolled,
                     int expected_scroll_height_after_scroll,
                     gfx::Vector2d expected_scroll_position_after_scroll,
                     bool jank_main_thread = false,
                     float page_scale_factor = 1) {
    EXPECT_EQ(0, GetScrollTop());

    int scroll_height =
        ExecuteScriptAndExtractInt("document.documentElement.scrollHeight");
    EXPECT_EQ(expected_scroll_height_after_scroll, scroll_height);

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

    if (jank_main_thread)
      JankMainThread();

    // Runs until we get the OnSyntheticGestureCompleted callback
    runner_->Run();
    runner_ = nullptr;

    gfx::Vector2dF root_scroll_offset =
        frame_watcher.LastMetadata().root_scroll_offset;
    if (jank_main_thread) {
      // Allow for 1px rounding inaccuracies
      EXPECT_LE(
          root_scroll_offset.y(),
          (expected_scroll_position_after_scroll.y() + 1) / page_scale_factor);
      EXPECT_GE(
          root_scroll_offset.y(),
          (expected_scroll_position_after_scroll.y() - 1) / page_scale_factor);
      EXPECT_LE(
          root_scroll_offset.x(),
          (expected_scroll_position_after_scroll.x() + 1) / page_scale_factor);
      EXPECT_GE(
          root_scroll_offset.x(),
          (expected_scroll_position_after_scroll.x() - 1) / page_scale_factor);
    }

    if (!jank_main_thread) {
      // GetScrollTop() and GetScrollLeft() goes through the main thread, here
      // we want to make sure that the compositor already scrolled before asking
      // the main thread.
      while (
          wait_until_scrolled &&
          root_scroll_offset.y() < expected_scroll_position_after_scroll.y() &&
          root_scroll_offset.x() < expected_scroll_position_after_scroll.x()) {
        frame_watcher.WaitFrames(1);
      }

      // Check the scroll offset
      int scroll_top = GetScrollTop();
      int scroll_left = GetScrollLeft();
      // Allow for 1px rounding inaccuracies for some screen sizes.
      EXPECT_LE(expected_scroll_position_after_scroll.y(), scroll_top);
      EXPECT_LE(expected_scroll_position_after_scroll.x(), scroll_left);
    }
  }

  void ResetLoadURLCalled() { load_url_called_ = false; }

 private:
  scoped_refptr<MessageLoopRunner> runner_;
  // Indicates whether the LoadURL() function has been called or not.
  bool load_url_called_ = false;

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

  DoTouchScroll(gfx::Point(50, 50), gfx::Vector2d(0, 45), true, 10300,
                gfx::Vector2d(0, 45));

  EXPECT_EQ(1, ExecuteScriptAndExtractInt("eventCounts.touchstart"));
  EXPECT_GE(ExecuteScriptAndExtractInt("eventCounts.touchmove"), 1);
  EXPECT_EQ(1, ExecuteScriptAndExtractInt("eventCounts.touchend"));
  EXPECT_EQ(0, ExecuteScriptAndExtractInt("eventCounts.touchcancel"));

  ResetLoadURLCalled();
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

  DoTouchScroll(gfx::Point(50, 150), gfx::Vector2d(0, 45), false, 10300,
                gfx::Vector2d(0, 0));

  EXPECT_EQ(1, ExecuteScriptAndExtractInt("eventCounts.touchstart"));
  EXPECT_GE(ExecuteScriptAndExtractInt("eventCounts.touchmove"), 1);
  EXPECT_EQ(1, ExecuteScriptAndExtractInt("eventCounts.touchend"));
  EXPECT_EQ(0, ExecuteScriptAndExtractInt("eventCounts.touchcancel"));

  ResetLoadURLCalled();
}

#if defined(OS_MACOSX)
#define MAYBE_PanYAllowed DISABLED_PanYAllowed
#else
#define MAYBE_PanYAllowed PanYAllowed
#endif
IN_PROC_BROWSER_TEST_F(TouchActionBrowserTest, MAYBE_PanYAllowed) {
  LoadURL(kTouchActionDataURL);

  DoTouchScroll(gfx::Point(50, 250), gfx::Vector2d(0, 45), true, 10300,
                gfx::Vector2d(0, 45));
}

#if defined(OS_MACOSX)
#define MAYBE_PanYMainThreadJanky DISABLED_PanYMainThreadJanky
#else
#define MAYBE_PanYMainThreadJanky PanYMainThreadJanky
#endif
IN_PROC_BROWSER_TEST_F(TouchActionBrowserTest, MAYBE_PanYMainThreadJanky) {
  LoadURL(kTouchActionJankURL);

  float page_scale_factor = GetPageScaleFactor();
  gfx::Point touch_point =
      gfx::Point(static_cast<float>(25) * page_scale_factor,
                 static_cast<float>(125) * page_scale_factor);
  DoTouchScroll(touch_point, gfx::Vector2d(0, 45), false, 10000,
                gfx::Vector2d(0, 45), true, page_scale_factor);

  ResetLoadURLCalled();
}

#if defined(OS_MACOSX)
#define MAYBE_PanXMainThreadJanky DISABLED_PanXMainThreadJanky
#else
#define MAYBE_PanXMainThreadJanky PanXMainThreadJanky
#endif
IN_PROC_BROWSER_TEST_F(TouchActionBrowserTest, MAYBE_PanXMainThreadJanky) {
  LoadURL(kTouchActionJankURL);

  float page_scale_factor = GetPageScaleFactor();
  gfx::Point touch_point =
      gfx::Point(static_cast<float>(125) * page_scale_factor,
                 static_cast<float>(25) * page_scale_factor);
  DoTouchScroll(touch_point, gfx::Vector2d(45, 0), false, 10000,
                gfx::Vector2d(45, 0), true, page_scale_factor);

  ResetLoadURLCalled();
}

#if defined(OS_MACOSX)
#define MAYBE_PanXYMainThreadJanky DISABLED_PanXYMainThreadJanky
#else
#define MAYBE_PanXYMainThreadJanky PanXYMainThreadJanky
#endif
IN_PROC_BROWSER_TEST_F(TouchActionBrowserTest, MAYBE_PanXYMainThreadJanky) {
  LoadURL(kTouchActionJankURL);

  float page_scale_factor = GetPageScaleFactor();
  gfx::Point touch_point =
      gfx::Point(static_cast<float>(75) * page_scale_factor,
                 static_cast<float>(60) * page_scale_factor);
  DoTouchScroll(touch_point, gfx::Vector2d(45, 45), false, 10000,
                gfx::Vector2d(45, 45), true, page_scale_factor);

  ResetLoadURLCalled();
}

#if defined(OS_MACOSX)
#define MAYBE_PanXYAtXAreaMainThreadJanky DISABLED_PanXYAtXAreaMainThreadJanky
#else
#define MAYBE_PanXYAtXAreaMainThreadJanky PanXYAtXAreaMainThreadJanky
#endif
IN_PROC_BROWSER_TEST_F(TouchActionBrowserTest,
                       MAYBE_PanXYAtXAreaMainThreadJanky) {
  LoadURL(kTouchActionJankURL);

  float page_scale_factor = GetPageScaleFactor();
  gfx::Point touch_point =
      gfx::Point(static_cast<float>(125) * page_scale_factor,
                 static_cast<float>(25) * page_scale_factor);
  DoTouchScroll(touch_point, gfx::Vector2d(45, 45), false, 10000,
                gfx::Vector2d(45, 0), true, page_scale_factor);

  ResetLoadURLCalled();
}

#if defined(OS_MACOSX)
#define MAYBE_PanXYAtYAreaMainThreadJanky DISABLED_PanXYAtYAreaMainThreadJanky
#else
#define MAYBE_PanXYAtYAreaMainThreadJanky PanXYAtYAreaMainThreadJanky
#endif
IN_PROC_BROWSER_TEST_F(TouchActionBrowserTest,
                       MAYBE_PanXYAtYAreaMainThreadJanky) {
  LoadURL(kTouchActionJankURL);

  float page_scale_factor = GetPageScaleFactor();
  gfx::Point touch_point =
      gfx::Point(static_cast<float>(25) * page_scale_factor,
                 static_cast<float>(125) * page_scale_factor);
  DoTouchScroll(touch_point, gfx::Vector2d(45, 45), false, 10000,
                gfx::Vector2d(0, 45), true, page_scale_factor);

  ResetLoadURLCalled();
}

#if defined(OS_MACOSX)
#define MAYBE_PanXYAtAutoYOverlapAreaMainThreadJanky \
  DISABLED_PanXYAtAutoYOverlapAreaMainThreadJanky
#else
#define MAYBE_PanXYAtAutoYOverlapAreaMainThreadJanky \
  PanXYAtAutoYOverlapAreaMainThreadJanky
#endif
IN_PROC_BROWSER_TEST_F(TouchActionBrowserTest,
                       MAYBE_PanXYAtAutoYOverlapAreaMainThreadJanky) {
  LoadURL(kTouchActionJankURL);

  float page_scale_factor = GetPageScaleFactor();
  gfx::Point touch_point =
      gfx::Point(static_cast<float>(75) * page_scale_factor,
                 static_cast<float>(125) * page_scale_factor);
  DoTouchScroll(touch_point, gfx::Vector2d(45, 45), false, 10000,
                gfx::Vector2d(0, 45), true, page_scale_factor);

  ResetLoadURLCalled();
}

#if defined(OS_MACOSX)
#define MAYBE_PanXYAtAutoXOverlapAreaMainThreadJanky \
  DISABLED_PanXYAtAutoXOverlapAreaMainThreadJanky
#else
#define MAYBE_PanXYAtAutoXOverlapAreaMainThreadJanky \
  PanXYAtAutoXOverlapAreaMainThreadJanky
#endif
IN_PROC_BROWSER_TEST_F(TouchActionBrowserTest,
                       MAYBE_PanXYAtAutoXOverlapAreaMainThreadJanky) {
  LoadURL(kTouchActionJankURL);

  float page_scale_factor = GetPageScaleFactor();
  gfx::Point touch_point =
      gfx::Point(static_cast<float>(125) * page_scale_factor,
                 static_cast<float>(75) * page_scale_factor);
  DoTouchScroll(touch_point, gfx::Vector2d(45, 45), false, 10000,
                gfx::Vector2d(45, 0), true, page_scale_factor);

  ResetLoadURLCalled();
}

}  // namespace content
