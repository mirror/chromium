// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/panels/panel_browser_window_cocoa.h"

#import <Cocoa/Cocoa.h>

#include "base/command_line.h"
#include "base/debug/debugger.h"
#include "base/memory/scoped_ptr.h"
#import "chrome/browser/ui/cocoa/browser_test_helper.h"
#import "chrome/browser/ui/cocoa/cocoa_test_helper.h"
#include "chrome/browser/ui/panels/panel.h"
#include "chrome/browser/ui/panels/panel_manager.h"
#import "chrome/browser/ui/panels/panel_titlebar_view_cocoa.h"
#import "chrome/browser/ui/panels/panel_window_controller_cocoa.h"
#include "chrome/common/chrome_switches.h"
#include "testing/gtest/include/gtest/gtest.h"

// Main test class.
class PanelBrowserWindowCocoaTest : public CocoaTest {
 public:
  virtual void SetUp() {
    CocoaTest::SetUp();
    CommandLine::ForCurrentProcess()->AppendSwitch(switches::kEnablePanels);
  }

  Panel* CreateTestPanel(const std::string& panel_name) {
    Browser* panel_browser = Browser::CreateForApp(Browser::TYPE_PANEL,
                                                   panel_name,
                                                   gfx::Rect(),
                                                   browser_helper_.profile());
    return static_cast<Panel*>(panel_browser->window());
  }

  void VerifyTitlebarLocation(NSView* contentView, NSView* titlebar) {
    NSRect content_frame = [contentView frame];
    NSRect titlebar_frame = [titlebar frame];
    // Since contentView and titlebar are both children of window's root view,
    // we can compare their frames since they are in the same coordinate system.
    EXPECT_EQ(NSMinX(content_frame), NSMinX(titlebar_frame));
    EXPECT_EQ(NSWidth(content_frame), NSWidth(titlebar_frame));
    EXPECT_EQ(NSMaxY(content_frame), NSMinY(titlebar_frame));
    EXPECT_EQ(NSHeight([[titlebar superview] bounds]), NSMaxY(titlebar_frame));
  }

 private:
  BrowserTestHelper browser_helper_;
};

TEST_F(PanelBrowserWindowCocoaTest, CreateClose) {
  PanelManager* manager = PanelManager::GetInstance();
  EXPECT_EQ(0, manager->active_count());  // No panels initially.

  Panel* panel = CreateTestPanel("Test Panel");
  EXPECT_TRUE(panel);
  EXPECT_TRUE(panel->native_panel());  // Native panel is created right away.
  PanelBrowserWindowCocoa* native_window =
      static_cast<PanelBrowserWindowCocoa*>(panel->native_panel());

  EXPECT_EQ(panel, native_window->panel_);  // Back pointer initialized.
  EXPECT_EQ(1, manager->active_count());

  // Window should not load before Show()
  EXPECT_FALSE([native_window->controller_ isWindowLoaded]);
  panel->Show();
  EXPECT_TRUE([native_window->controller_ isWindowLoaded]);
  EXPECT_TRUE([native_window->controller_ window]);

  gfx::Rect bounds = panel->GetBounds();
  EXPECT_TRUE(bounds.width() > 0);
  EXPECT_TRUE(bounds.height() > 0);

  // NSWindows created by NSWindowControllers don't have this bit even if
  // their NIB has it. The controller's lifetime is the window's lifetime.
  EXPECT_EQ(NO, [[native_window->controller_ window] isReleasedWhenClosed]);

  panel->Close();
  EXPECT_EQ(0, manager->active_count());
  // Close() destroys the controller, which destroys the NSWindow. CocoaTest
  // base class verifies that there is no remaining open windows after the test.
  EXPECT_FALSE(native_window->controller_);
}

TEST_F(PanelBrowserWindowCocoaTest, AssignedBounds) {
  PanelManager* manager = PanelManager::GetInstance();
  Panel* panel1 = CreateTestPanel("Test Panel 1");
  Panel* panel2 = CreateTestPanel("Test Panel 2");
  Panel* panel3 = CreateTestPanel("Test Panel 3");
  EXPECT_EQ(3, manager->active_count());

  panel1->Show();
  panel2->Show();
  panel3->Show();

  gfx::Rect bounds1 = panel1->GetBounds();
  gfx::Rect bounds2 = panel2->GetBounds();
  gfx::Rect bounds3 = panel3->GetBounds();

  // This checks panelManager calculating and assigning bounds right.
  // Panels should stack on the bottom right to left.
  EXPECT_LT(bounds3.x() + bounds3.width(), bounds2.x());
  EXPECT_LT(bounds2.x() + bounds2.width(), bounds1.x());
  EXPECT_EQ(bounds1.y(), bounds2.y());
  EXPECT_EQ(bounds2.y(), bounds3.y());

  // After panel2 is closed, panel3 should take its place.
  panel2->Close();
  bounds3 = panel3->GetBounds();
  EXPECT_EQ(bounds2, bounds3);
  EXPECT_EQ(2, manager->active_count());

  // After panel1 is closed, panel3 should take its place.
  panel1->Close();
  EXPECT_EQ(bounds1, panel3->GetBounds());
  EXPECT_EQ(1, manager->active_count());

  panel3->Close();
  EXPECT_EQ(0, manager->active_count());
}

// Same test as AssignedBounds, but checks actual bounds on native OS windows.
TEST_F(PanelBrowserWindowCocoaTest, NativeBounds) {
  PanelManager* manager = PanelManager::GetInstance();
  Panel* panel1 = CreateTestPanel("Test Panel 1");
  Panel* panel2 = CreateTestPanel("Test Panel 2");
  Panel* panel3 = CreateTestPanel("Test Panel 3");
  EXPECT_EQ(3, manager->active_count());

  panel1->Show();
  panel2->Show();
  panel3->Show();

  PanelBrowserWindowCocoa* native_window1 =
      static_cast<PanelBrowserWindowCocoa*>(panel1->native_panel());
  PanelBrowserWindowCocoa* native_window2 =
      static_cast<PanelBrowserWindowCocoa*>(panel2->native_panel());
  PanelBrowserWindowCocoa* native_window3 =
      static_cast<PanelBrowserWindowCocoa*>(panel3->native_panel());

  NSRect bounds1 = [[native_window1->controller_ window] frame];
  NSRect bounds2 = [[native_window2->controller_ window] frame];
  NSRect bounds3 = [[native_window3->controller_ window] frame];

  EXPECT_LT(bounds3.origin.x + bounds3.size.width, bounds2.origin.x);
  EXPECT_LT(bounds2.origin.x + bounds2.size.width, bounds1.origin.x);
  EXPECT_EQ(bounds1.origin.y, bounds2.origin.y);
  EXPECT_EQ(bounds2.origin.y, bounds3.origin.y);

  // After panel2 is closed, panel3 should take its place.
  panel2->Close();
  bounds3 = [[native_window3->controller_ window] frame];
  EXPECT_EQ(bounds2.origin.x, bounds3.origin.x);
  EXPECT_EQ(bounds2.origin.y, bounds3.origin.y);
  EXPECT_EQ(bounds2.size.width, bounds3.size.width);
  EXPECT_EQ(bounds2.size.height, bounds3.size.height);
  EXPECT_EQ(2, manager->active_count());

  // After panel1 is closed, panel3 should take its place.
  panel1->Close();
  bounds3 = [[native_window3->controller_ window] frame];
  EXPECT_EQ(bounds1.origin.x, bounds3.origin.x);
  EXPECT_EQ(bounds1.origin.y, bounds3.origin.y);
  EXPECT_EQ(bounds1.size.width, bounds3.size.width);
  EXPECT_EQ(bounds1.size.height, bounds3.size.height);
  EXPECT_EQ(1, manager->active_count());

  panel3->Close();
  EXPECT_EQ(0, manager->active_count());
}

// Verify the titlebar is being created.
TEST_F(PanelBrowserWindowCocoaTest, TitlebarViewCreate) {
  Panel* panel = CreateTestPanel("Test Panel");
  panel->Show();

  PanelBrowserWindowCocoa* native_window =
      static_cast<PanelBrowserWindowCocoa*>(panel->native_panel());

  PanelTitlebarViewCocoa* titlebar = [native_window->controller_ titlebarView];
  EXPECT_TRUE(titlebar);
  EXPECT_EQ(native_window->controller_, [titlebar controller]);

  panel->Close();
}

// Verify the sizing of titlebar - should be affixed on top of regular titlebar.
TEST_F(PanelBrowserWindowCocoaTest, TitlebarViewSizing) {
  Panel* panel = CreateTestPanel("Test Panel");
  panel->Show();

  PanelBrowserWindowCocoa* native_window =
      static_cast<PanelBrowserWindowCocoa*>(panel->native_panel());
  PanelTitlebarViewCocoa* titlebar = [native_window->controller_ titlebarView];

  NSView* contentView = [[native_window->controller_ window] contentView];
  VerifyTitlebarLocation(contentView, titlebar);

  // In local coordinate system, width of titlebar should match width of
  // content view of the window. They both use the same scale factor.
  EXPECT_EQ(NSWidth([contentView bounds]), NSWidth([titlebar bounds]));

  // Now resize the Panel, see that titlebar follows.
  const int kDelta = 153; // random number
  gfx::Rect bounds = panel->GetBounds();
  // Grow panel in a way so that its titlebar moves and grows.
  bounds.set_x(bounds.x() - kDelta);
  bounds.set_y(bounds.y() - kDelta);
  bounds.set_width(bounds.width() + kDelta);
  bounds.set_height(bounds.height() + kDelta);
  native_window->SetPanelBounds(bounds);

  // Verify the panel resized.
  NSRect window_frame = [[native_window->controller_ window] frame];
  EXPECT_EQ(NSWidth(window_frame), bounds.width());
  EXPECT_EQ(NSHeight(window_frame), bounds.height());

  // Verify the titlebar is still on top of regular titlebar.
  VerifyTitlebarLocation(contentView, titlebar);

  panel->Close();
}

// Verify closing behavior of titlebar close button.
TEST_F(PanelBrowserWindowCocoaTest, TitlebarViewClose) {
  PanelManager* manager = PanelManager::GetInstance();
  Panel* panel = CreateTestPanel("Test Panel");
  panel->Show();

  PanelBrowserWindowCocoa* native_window =
      static_cast<PanelBrowserWindowCocoa*>(panel->native_panel());

  PanelTitlebarViewCocoa* titlebar = [native_window->controller_ titlebarView];
  EXPECT_TRUE(titlebar);

  EXPECT_EQ(1, manager->active_count());
  // Simulate clicking Close Button. This should close the Panel as well.
  [titlebar simulateCloseButtonClick];
  EXPECT_EQ(0, manager->active_count());
}

