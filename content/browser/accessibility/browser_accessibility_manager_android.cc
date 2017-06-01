// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/accessibility/browser_accessibility_manager_android.h"

#include <stddef.h>

#include <cmath>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/feature_list.h"
#include "base/i18n/char_iterator.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "content/app/strings/grit/content_strings.h"
#include "content/browser/accessibility/browser_accessibility_android.h"
#include "content/browser/accessibility/one_shot_accessibility_tree_search.h"
#include "content/browser/accessibility/web_contents_accessibility.h"
#include "content/common/accessibility_messages.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_features.h"
#include "ui/accessibility/ax_text_utils.h"

using base::android::AttachCurrentThread;
using base::android::ScopedJavaLocalRef;

namespace content {

// static
BrowserAccessibilityManager* BrowserAccessibilityManager::Create(
    const ui::AXTreeUpdate& initial_tree,
    BrowserAccessibilityDelegate* delegate,
    BrowserAccessibilityFactory* factory) {
  return new BrowserAccessibilityManagerAndroid(initial_tree, delegate,
                                                factory);
}

BrowserAccessibilityManagerAndroid::BrowserAccessibilityManagerAndroid(
    const ui::AXTreeUpdate& initial_tree,
    BrowserAccessibilityDelegate* delegate,
    BrowserAccessibilityFactory* factory)
    : BrowserAccessibilityManager(delegate, factory),
      prune_tree_for_screen_reader_(true) {
  Initialize(initial_tree);
}

BrowserAccessibilityManagerAndroid::~BrowserAccessibilityManagerAndroid() {}

bool BrowserAccessibilityManagerAndroid::ShouldRespectDisplayedPasswordText() {
  auto* root_manager = GetManagerJni();
  return root_manager ? root_manager->ShouldRespectDisplayedPasswordTextJni()
                      : false;
}

bool BrowserAccessibilityManagerAndroid::ShouldExposePasswordText() {
  auto* root_manager = GetManagerJni();
  return root_manager ? root_manager->ShouldExposePasswordTextJni() : false;
}

BrowserAccessibility* BrowserAccessibilityManagerAndroid::GetFocus() {
  BrowserAccessibility* focus = BrowserAccessibilityManager::GetFocus();
  return GetActiveDescendant(focus);
}

void BrowserAccessibilityManagerAndroid::NotifyAccessibilityEvent(
    BrowserAccessibilityEvent::Source source,
    ui::AXEvent event_type,
    BrowserAccessibility* node) {
  auto* root_manager = GetManagerJni();
  if (!root_manager)
    return;

  if (event_type == ui::AX_EVENT_HIDE)
    return;

  if (event_type == ui::AX_EVENT_TREE_CHANGED)
    return;

  // Layout changes are handled in OnLocationChanges and
  // SendLocationChangeEvents.
  if (event_type == ui::AX_EVENT_LAYOUT_COMPLETE)
    return;

  if (event_type == ui::AX_EVENT_HOVER) {
    HandleHoverEvent(node);
    return;
  }

  // Sometimes we get events on nodes in our internal accessibility tree
  // that aren't exposed on Android. Update |node| to point to the highest
  // ancestor that's a leaf node.
  BrowserAccessibility* original_node = node;
  node = node->GetClosestPlatformObject();
  BrowserAccessibilityAndroid* android_node =
      static_cast<BrowserAccessibilityAndroid*>(node);

  // If the closest platform object is a password field, the event we're
  // getting is doing something in the shadow dom, for example replacing a
  // character with a dot after a short pause. On Android we don't want to
  // fire an event for those changes, but we do want to make sure our internal
  // state is correct, so we call OnDataChanged() and then return.
  if (android_node->IsPassword() && original_node != node) {
    android_node->OnDataChanged();
    return;
  }

  // Always send AccessibilityEvent.TYPE_WINDOW_CONTENT_CHANGED to notify
  // the Android system that the accessibility hierarchy rooted at this
  // node has changed.
  root_manager->HandleContentChanged(node->unique_id());

  // Ignore load complete events on iframes.
  if (event_type == ui::AX_EVENT_LOAD_COMPLETE &&
      node->manager() != GetRootManager()) {
    return;
  }

  switch (event_type) {
    case ui::AX_EVENT_LOAD_COMPLETE:
      root_manager->HandlePageLoaded(GetFocus()->unique_id());
      break;
    case ui::AX_EVENT_FOCUS:
      root_manager->HandleFocusChanged(node->unique_id());
      break;
    case ui::AX_EVENT_CHECKED_STATE_CHANGED:
      root_manager->HandleCheckStateChanged(node->unique_id());
      break;
    case ui::AX_EVENT_CLICKED:
      root_manager->HandleClicked(node->unique_id());
      break;
    case ui::AX_EVENT_SCROLL_POSITION_CHANGED:
      root_manager->HandleScrollPositionChanged(node->unique_id());
      break;
    case ui::AX_EVENT_SCROLLED_TO_ANCHOR:
      root_manager->HandleScrolledToAnchor(node->unique_id());
      break;
    case ui::AX_EVENT_ALERT:
      // An alert is a special case of live region. Fall through to the
      // next case to handle it.
    case ui::AX_EVENT_SHOW: {
      // This event is fired when an object appears in a live region.
      // Speak its text.
      base::string16 text = android_node->GetText();
      root_manager->AnnounceLiveRegionText(text);
      break;
    }
    case ui::AX_EVENT_TEXT_SELECTION_CHANGED:
      root_manager->HandleTextSelectionChanged(node->unique_id());
      break;
    case ui::AX_EVENT_TEXT_CHANGED:
    case ui::AX_EVENT_VALUE_CHANGED:
      if (android_node->IsEditableText() && GetFocus() == node) {
        root_manager->HandleEditableTextChanged(node->unique_id());
      } else if (android_node->IsSlider()) {
        root_manager->HandleSliderChanged(node->unique_id());
      }
      break;
    default:
      // There are some notifications that aren't meaningful on Android.
      // It's okay to skip them.
      break;
  }
}

void BrowserAccessibilityManagerAndroid::SendLocationChangeEvents(
      const std::vector<AccessibilityHostMsg_LocationChangeParams>& params) {
  // Android is not very efficient at handling notifications, and location
  // changes in particular are frequent and not time-critical. If a lot of
  // nodes changed location, just send a single notification after a short
  // delay (to batch them), rather than lots of individual notifications.
  if (params.size() <= 3) {
    auto* root_manager = GetManagerJni();
    if (!root_manager)
      return;
    root_manager->SendDelayedWindowContentChangedEvent();
  }
  BrowserAccessibilityManager::SendLocationChangeEvents(params);
}

void BrowserAccessibilityManagerAndroid::HandleHoverEvent(
    BrowserAccessibility* node) {
  auto* root_manager = GetManagerJni();
  if (!root_manager)
    return;

  // First walk up to the nearest platform node, in case this node isn't
  // even exposed on the platform.
  node = node->GetClosestPlatformObject();

  // If this node is uninteresting and just a wrapper around a sole
  // interesting descendant, prefer that descendant instead.
  const BrowserAccessibilityAndroid* android_node =
      static_cast<BrowserAccessibilityAndroid*>(node);
  const BrowserAccessibilityAndroid* sole_interesting_node =
      android_node->GetSoleInterestingNodeFromSubtree();
  if (sole_interesting_node)
    android_node = sole_interesting_node;

  // Finally, if this node is still uninteresting, try to walk up to
  // find an interesting parent.
  while (android_node && !android_node->IsInterestingOnAndroid()) {
    android_node = static_cast<BrowserAccessibilityAndroid*>(
        android_node->PlatformGetParent());
  }

  if (android_node) {
    root_manager->HandleHover(android_node->unique_id());
  }
}

void BrowserAccessibilityManagerAndroid::OnAtomicUpdateFinished(
    ui::AXTree* tree,
    bool root_changed,
    const std::vector<ui::AXTreeDelegate::Change>& changes) {
  BrowserAccessibilityManager::OnAtomicUpdateFinished(
      tree, root_changed, changes);

  if (root_changed) {
    auto* root_manager = GetManagerJni();
    if (!root_manager)
      return;

    root_manager->HandleNavigate();
  }
}

bool
BrowserAccessibilityManagerAndroid::UseRootScrollOffsetsWhenComputingBounds() {
  // The Java layer handles the root scroll offset.
  return false;
}

WebContentsAccessibility* BrowserAccessibilityManagerAndroid::GetManagerJni() {
  return static_cast<WebContentsAccessibility*>(GetRootManager());
}

}  // namespace content
