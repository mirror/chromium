// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "content/browser/accessibility/browser_accessibility.h"
#include "content/browser/accessibility/browser_accessibility_manager.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "content/shell/browser/shell.h"
#include "content/test/accessibility_browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

class AccessibilityHitTestingBrowserTest : public ContentBrowserTest {
 public:
  AccessibilityHitTestingBrowserTest() {}
  ~AccessibilityHitTestingBrowserTest() override {}

 protected:
  BrowserAccessibility* FindButton(BrowserAccessibility* node) {
    if (node->GetRole() == ui::AX_ROLE_BUTTON)
      return node;
    for (unsigned i = 0; i < node->PlatformChildCount(); i++) {
      if (BrowserAccessibility* button = FindButton(node->PlatformGetChild(i)))
        return button;
    }
    return nullptr;
  }

  BrowserAccessibility* HitTestAndWaitForResultWithEvent(
      const gfx::Point& point,
      ui::AXEvent event_to_fire) {
    WebContentsImpl* web_contents =
        static_cast<WebContentsImpl*>(shell()->web_contents());
    FrameTree* frame_tree = web_contents->GetFrameTree();
    BrowserAccessibilityManager* manager =
        web_contents->GetRootBrowserAccessibilityManager();

    AccessibilityNotificationWaiter event_waiter(
        shell()->web_contents(), ui::kAXModeComplete, event_to_fire);
    for (FrameTreeNode* node : frame_tree->Nodes())
      event_waiter.ListenToAdditionalFrame(node->current_frame_host());
    ui::AXActionData action_data;
    action_data.action = ui::AX_ACTION_HIT_TEST;
    action_data.target_point = point;
    action_data.hit_test_event_to_fire = event_to_fire;
    manager->delegate()->AccessibilityPerformAction(action_data);
    event_waiter.WaitForNotification();

    RenderFrameHostImpl* target_frame = event_waiter.event_render_frame_host();
    BrowserAccessibilityManager* target_manager =
        target_frame->browser_accessibility_manager();
    int event_target_id = event_waiter.event_target_id();
    BrowserAccessibility* hit_node = target_manager->GetFromID(event_target_id);
    return hit_node;
  }

  BrowserAccessibility* HitTestAndWaitForResult(const gfx::Point& point) {
    return HitTestAndWaitForResultWithEvent(point, ui::AX_EVENT_HOVER);
  }

  BrowserAccessibility* CallCachingAsyncHitTest(const gfx::Point& point) {
    WebContentsImpl* web_contents =
        static_cast<WebContentsImpl*>(shell()->web_contents());
    FrameTree* frame_tree = web_contents->GetFrameTree();
    BrowserAccessibilityManager* manager =
        web_contents->GetRootBrowserAccessibilityManager();
    gfx::Point screen_point =
        point + manager->GetViewBounds().OffsetFromOrigin();

    // Each call to CachingAsyncHitTest results in at least one HOVER
    // event received. Block until we receive it.
    AccessibilityNotificationWaiter hover_waiter(
        shell()->web_contents(), ui::kAXModeComplete, ui::AX_EVENT_HOVER);
    for (FrameTreeNode* node : frame_tree->Nodes())
      hover_waiter.ListenToAdditionalFrame(node->current_frame_host());
    BrowserAccessibility* result = manager->CachingAsyncHitTest(screen_point);
    hover_waiter.WaitForNotification();
    return result;
  }
};

IN_PROC_BROWSER_TEST_F(AccessibilityHitTestingBrowserTest, Fullscreen) {
  ASSERT_TRUE(embedded_test_server()->Start());

  NavigateToURL(shell(), GURL(url::kAboutBlankURL));

  AccessibilityNotificationWaiter waiter(
      shell()->web_contents(), ui::kAXModeComplete, ui::AX_EVENT_LOAD_COMPLETE);
  GURL url(
      embedded_test_server()->GetURL("/accessibility/html/fullscreen.html"));
  NavigateToURL(shell(), url);
  waiter.WaitForNotification();

  WebContentsImpl* web_contents =
      static_cast<WebContentsImpl*>(shell()->web_contents());
  BrowserAccessibilityManager* manager =
      web_contents->GetRootBrowserAccessibilityManager();
  BrowserAccessibility* button = FindButton(manager->GetRoot());

  ASSERT_TRUE(button != nullptr);

  manager->DoDefaultAction(*button);

  WaitForAccessibilityTreeToContainNodeWithName(web_contents, "Done");
}

}  // namespace content
