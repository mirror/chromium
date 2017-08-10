// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <Cocoa/Cocoa.h>

#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/cocoa/view_id_util.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "testing/gtest_mac.h"

class AccessibilityTest : public InProcessBrowserTest {
 public:
  AccessibilityTest() {}

 protected:
  NSView* ContentView() {
    return [browser()->window()->GetNativeWindow() contentView];
  }

  NSView* ViewWithID(ViewID id) {
    return view_id_util::GetView(browser()->window()->GetNativeWindow(), id);
  }

  // For NSView subclasses with cells, the cell contains the accessibility
  // properties, not the view. NSButton is one such subclass; there are probably
  // others as well.
  NSObject* RealAXObjectFor(NSObject* object) {
    if ([object isKindOfClass:[NSButton class]])
      return [static_cast<NSButton*>(object) cell];
    return object;
  }

  id AXAttribute(NSObject* obj, NSString* attribute) {
    return [RealAXObjectFor(obj) accessibilityAttributeValue:attribute];
  }

  bool AXIsIgnored(NSObject* obj) {
    return [RealAXObjectFor(obj) accessibilityIsIgnored];
  }

  NSString* AXRole(NSObject* obj) {
    return AXAttribute(obj, NSAccessibilityRoleAttribute);
  }

  NSArray* AXChildren(NSObject* obj) {
    return AXAttribute(obj, NSAccessibilityChildrenAttribute);
  }

  NSString* AXTitle(NSObject* obj) {
    return AXAttribute(obj, NSAccessibilityTitleAttribute);
  }

  NSString* AXDescription(NSObject* obj) {
    return AXAttribute(obj, NSAccessibilityDescriptionAttribute);
  }

  // Computes the entire accessibility tree rooted at |root|, not including
  // ignored nodes (those for which AXIsIgnored() returns true) but including
  // children of ignored nodes.
  NSArray* DeepAXTree(NSObject* root) {
    NSMutableArray* result = [[[NSMutableArray alloc] init] autorelease];
    DeepAXTreeHelper(root, result);
    return result;
  }

 private:
  void DeepAXTreeHelper(NSObject* root, NSMutableArray* result) {
    if (!AXIsIgnored(root))
      [result addObject:root];
    for (NSObject* child : AXChildren(root))
      DeepAXTreeHelper(child, result);
  }
};

IN_PROC_BROWSER_TEST_F(AccessibilityTest, EveryElementHasRole) {
  NSArray* elements = DeepAXTree(ContentView());
  for (NSObject* elem : elements) {
    EXPECT_NSNE(AXRole(elem), NSAccessibilityUnknownRole);
  }
}

// Every element must have at least one of a title or a description.
IN_PROC_BROWSER_TEST_F(AccessibilityTest, EveryElementHasText) {
  NSArray* elements = DeepAXTree(ContentView());
  for (NSObject* elem : elements) {
    NSString* text =
        [AXTitle(elem) length] > 0 ? AXTitle(elem) : AXDescription(elem);
    EXPECT_NSNE(text, @"");
  }
}

IN_PROC_BROWSER_TEST_F(AccessibilityTest, ControlRoles) {
  // TODO(ellyjones): figure out a more principled way to ensure each control
  // has the expected role. For now, this is just a regression test for
  // https://crbug.com/754223
  EXPECT_NSEQ(AXRole(ViewWithID(VIEW_ID_APP_MENU)),
              NSAccessibilityPopUpButtonRole);
}
