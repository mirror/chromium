// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/accessibility/browser_accessibility_manager_auralinux.h"

#include "content/browser/accessibility/browser_accessibility_auralinux.h"
#include "content/common/accessibility_messages.h"
#include "ui/accessibility/platform/ax_platform_node_auralinux.h"

namespace content {

// static
BrowserAccessibilityManager* BrowserAccessibilityManager::Create(
    const ui::AXTreeUpdate& initial_tree,
    BrowserAccessibilityDelegate* delegate,
    BrowserAccessibilityFactory* factory) {
  return new BrowserAccessibilityManagerAuraLinux(nullptr, initial_tree,
                                                  delegate, factory);
}

BrowserAccessibilityManagerAuraLinux*
BrowserAccessibilityManager::ToBrowserAccessibilityManagerAuraLinux() {
  return static_cast<BrowserAccessibilityManagerAuraLinux*>(this);
}

BrowserAccessibilityManagerAuraLinux::BrowserAccessibilityManagerAuraLinux(
    AtkObject* parent_object,
    const ui::AXTreeUpdate& initial_tree,
    BrowserAccessibilityDelegate* delegate,
    BrowserAccessibilityFactory* factory)
    : BrowserAccessibilityManager(delegate, factory),
      parent_object_(parent_object) {
  Initialize(initial_tree);
}

BrowserAccessibilityManagerAuraLinux::~BrowserAccessibilityManagerAuraLinux() {
}

// static
ui::AXTreeUpdate
    BrowserAccessibilityManagerAuraLinux::GetEmptyDocument() {
  ui::AXNodeData empty_document;
  empty_document.id = 0;
  empty_document.role = ui::AX_ROLE_ROOT_WEB_AREA;
  ui::AXTreeUpdate update;
  update.root_id = empty_document.id;
  update.nodes.push_back(empty_document);
  return update;
}

void BrowserAccessibilityManagerAuraLinux::FireFocusEvent(
    BrowserAccessibility* node) {
  BrowserAccessibilityManager::FireFocusEvent(node);
  if (node->IsNative()) {
    ToBrowserAccessibilityAuraLinux(node)->GetNode()->NotifyAccessibilityEvent(
        ui::AX_EVENT_FOCUS);
  }
}

void BrowserAccessibilityManagerAuraLinux::FireBlinkEvent(
    ui::AXEvent event_type,
    BrowserAccessibility* node) {
  BrowserAccessibilityManager::FireBlinkEvent(event_type, node);
  // Need to implement.
}

void BrowserAccessibilityManagerAuraLinux::FireGeneratedEvent(
    AXEventGenerator::Event event_type,
    BrowserAccessibility* node) {
  BrowserAccessibilityManager::FireGeneratedEvent(event_type, node);

  switch (event_type) {
    case Event::ROLE_CHANGED:
    case Event::CHILDREN_CHANGED:
      // Not needed. These are already handled in
      // AXPlatformNodeAuraLinux.
      break;
    case Event::ACTIVE_DESCENDANT_CHANGED:
    case Event::ALERT:
    case Event::DOCUMENT_SELECTION_CHANGED:
    case Event::CHECKED_STATE_CHANGED:
    case Event::COLLAPSED:
    case Event::DESCRIPTION_CHANGED:
    case Event::DOCUMENT_TITLE_CHANGED:
    case Event::EXPANDED:
    case Event::INVALID_STATUS_CHANGED:
    case Event::LIVE_REGION_CHANGED:
    case Event::LIVE_REGION_CREATED:
    case Event::LIVE_REGION_NODE_CHANGED:
    case Event::LOAD_COMPLETE:
    case Event::MENU_ITEM_SELECTED:
    case Event::NAME_CHANGED:
    case Event::OTHER_ATTRIBUTE_CHANGED:
    case Event::ROW_COUNT_CHANGED:
    case Event::SCROLL_POSITION_CHANGED:
    case Event::SELECTED_CHANGED:
    case Event::SELECTED_CHILDREN_CHANGED:
    case Event::STATE_CHANGED:
    case Event::VALUE_CHANGED:
      // Not implemented.
      break;
  }
}

}  // namespace content
