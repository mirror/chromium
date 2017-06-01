// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ACCESSIBILITY_BROWSER_ACCESSIBILITY_MANAGER_ANDROID_H_
#define CONTENT_BROWSER_ACCESSIBILITY_BROWSER_ACCESSIBILITY_MANAGER_ANDROID_H_

#include <stdint.h>

#include "base/android/scoped_java_ref.h"
#include "base/macros.h"
#include "content/browser/accessibility/browser_accessibility_manager.h"

namespace content {

// A Java counterpart will be generated for this enum.
// GENERATED_JAVA_ENUM_PACKAGE: org.chromium.content.browser.accessibility
enum ScrollDirection {
  FORWARD,
  BACKWARD,
  UP,
  DOWN,
  LEFT,
  RIGHT
};

// From android.view.accessibility.AccessibilityNodeInfo in Java:
enum AndroidMovementGranularity {
  ANDROID_ACCESSIBILITY_NODE_INFO_MOVEMENT_GRANULARITY_CHARACTER = 1,
  ANDROID_ACCESSIBILITY_NODE_INFO_MOVEMENT_GRANULARITY_WORD = 2,
  ANDROID_ACCESSIBILITY_NODE_INFO_MOVEMENT_GRANULARITY_LINE = 4
};

// From android.view.accessibility.AccessibilityEvent in Java:
enum {
  ANDROID_ACCESSIBILITY_EVENT_TEXT_CHANGED = 16,
  ANDROID_ACCESSIBILITY_EVENT_TEXT_SELECTION_CHANGED = 8192,
  ANDROID_ACCESSIBILITY_EVENT_TEXT_TRAVERSED_AT_MOVEMENT_GRANULARITY = 131072
};

class WebContentsAccessibility;

class CONTENT_EXPORT BrowserAccessibilityManagerAndroid
    : public BrowserAccessibilityManager {
 public:
  BrowserAccessibilityManagerAndroid(
      const ui::AXTreeUpdate& initial_tree,
      BrowserAccessibilityDelegate* delegate,
      BrowserAccessibilityFactory* factory = new BrowserAccessibilityFactory());
  ~BrowserAccessibilityManagerAndroid() override;

  // By default, the tree is pruned for a better screen reading experience,
  // including:
  //   * If the node has only static text children
  //   * If the node is focusable and has no focusable children
  //   * If the node is a heading
  // This can be turned off to generate a tree that more accurately reflects
  // the DOM and includes style changes within these nodes.
  void set_prune_tree_for_screen_reader(bool prune) {
    prune_tree_for_screen_reader_ = prune;
  }
  bool prune_tree_for_screen_reader() { return prune_tree_for_screen_reader_; }

  bool ShouldRespectDisplayedPasswordText();
  bool ShouldExposePasswordText();

  // BrowserAccessibilityManager overrides.
  BrowserAccessibility* GetFocus() override;
  void NotifyAccessibilityEvent(
      BrowserAccessibilityEvent::Source source,
      ui::AXEvent event_type,
      BrowserAccessibility* node) override;
  void SendLocationChangeEvents(
      const std::vector<AccessibilityHostMsg_LocationChangeParams>& params)
          override;

 protected:
  // AXTreeDelegate overrides.
  void OnAtomicUpdateFinished(
      ui::AXTree* tree,
      bool root_changed,
      const std::vector<ui::AXTreeDelegate::Change>& changes) override;

  bool UseRootScrollOffsetsWhenComputingBounds() override;

 private:
  virtual WebContentsAccessibility* GetManagerJni();

  // This gives BrowserAccessibilityManager::Create access to the class
  // constructor.
  friend class BrowserAccessibilityManager;

  // Handle a hover event from the renderer process.
  void HandleHoverEvent(BrowserAccessibility* node);

  // See docs for set_prune_tree_for_screen_reader, above.
  bool prune_tree_for_screen_reader_;

  DISALLOW_COPY_AND_ASSIGN(BrowserAccessibilityManagerAndroid);
};

}

#endif  // CONTENT_BROWSER_ACCESSIBILITY_BROWSER_ACCESSIBILITY_MANAGER_ANDROID_H_
