// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/extensions/chrome_extension_messages.h"
#include "chrome/renderer/extensions/automation_internal_custom_bindings.h"
#include "ui/accessibility/ax_node.h"

namespace extensions {

AutomationAXTreeWrapper::AutomationAXTreeWrapper(
    int32_t tree_id,
    AutomationInternalCustomBindings* owner)
    : AXEventGenerator(nullptr),
      tree_id_(tree_id),
      host_node_id_(-1),
      owner_(owner) {
  // We have to initialize AXEventGenerator here - we can't do it in the
  // initializer list because AXTree hasn't been initialized yet at that point.
  SetTree(&tree_);
}

AutomationAXTreeWrapper::~AutomationAXTreeWrapper() {
  // Clearing the delegate so we don't get a callback for every node
  // being deleted.
  tree_.SetDelegate(nullptr);
}

bool AutomationAXTreeWrapper::OnAccessibilityEvent(
    const ExtensionMsg_AccessibilityEventParams& params,
    bool is_active_profile) {
  deleted_node_ids_.clear();

  bool success = tree_.Unserialize(params.update);
  if (!success)
    return false;

  // Don't send any events if it's not the active profile.
  if (!is_active_profile)
    return true;

  owner_->SendNodesRemovedEvent(&tree_, deleted_node_ids_);
  deleted_node_ids_.clear();

  // Send events directly from the event message.
  if (ShouldSendAutomationEvent(params.event_type)) {
    owner_->SendAutomationEvent(params, params.id,
                                ui::ToString(params.event_type));
  }

  // Send auto-generated AXEventGenerator events. Any event explicitly fired
  // here should be suppressed in ShouldSendAutomationEvent(), below.
  for (auto targeted_event : *this) {
    api::automation::EventType event_type = api::automation::EVENT_TYPE_NONE;
    switch (targeted_event.event) {
      case Event::LOAD_COMPLETE:
        event_type = api::automation::EVENT_TYPE_LOADCOMPLETE;
        break;
      default:
        break;
    }

    if (event_type == extensions::api::automation::EVENT_TYPE_NONE)
      continue;

    owner_->SendAutomationEvent(params, targeted_event.node->id(),
                                api::automation::ToString(event_type));
  }
  ClearEvents();

  return true;
}

void AutomationAXTreeWrapper::OnNodeDataWillChange(
    ui::AXTree* tree,
    const ui::AXNodeData& old_node_data,
    const ui::AXNodeData& new_node_data) {
  AXEventGenerator::OnNodeDataWillChange(tree, old_node_data, new_node_data);
  if (old_node_data.GetStringAttribute(ui::AX_ATTR_NAME) !=
      new_node_data.GetStringAttribute(ui::AX_ATTR_NAME))
    text_changed_node_ids_.push_back(new_node_data.id);
}

void AutomationAXTreeWrapper::OnNodeWillBeDeleted(ui::AXTree* tree,
                                                  ui::AXNode* node) {
  AXEventGenerator::OnNodeWillBeDeleted(tree, node);
  owner_->SendTreeChangeEvent(api::automation::TREE_CHANGE_TYPE_NODEREMOVED,
                              tree, node);
  deleted_node_ids_.push_back(node->id());
}

void AutomationAXTreeWrapper::OnAtomicUpdateFinished(
    ui::AXTree* tree,
    bool root_changed,
    const std::vector<ui::AXTreeDelegate::Change>& changes) {
  AXEventGenerator::OnAtomicUpdateFinished(tree, root_changed, changes);
  DCHECK_EQ(&tree_, tree);
  for (const auto change : changes) {
    ui::AXNode* node = change.node;
    switch (change.type) {
      case NODE_CREATED:
        owner_->SendTreeChangeEvent(
            api::automation::TREE_CHANGE_TYPE_NODECREATED, tree, node);
        break;
      case SUBTREE_CREATED:
        owner_->SendTreeChangeEvent(
            api::automation::TREE_CHANGE_TYPE_SUBTREECREATED, tree, node);
        break;
      case NODE_CHANGED:
        owner_->SendTreeChangeEvent(
            api::automation::TREE_CHANGE_TYPE_NODECHANGED, tree, node);
        break;
      // Unhandled.
      case NODE_REPARENTED:
      case SUBTREE_REPARENTED:
        break;
    }
  }

  for (int id : text_changed_node_ids_) {
    owner_->SendTreeChangeEvent(api::automation::TREE_CHANGE_TYPE_TEXTCHANGED,
                                tree, tree->GetFromId(id));
  }
  text_changed_node_ids_.clear();
}

bool AutomationAXTreeWrapper::ShouldSendAutomationEvent(
    ui::AXEvent event) const {
  switch (event) {
    case ui::AX_EVENT_ACTIVEDESCENDANTCHANGED:
    case ui::AX_EVENT_ALERT:
    case ui::AX_EVENT_ARIA_ATTRIBUTE_CHANGED:
    case ui::AX_EVENT_AUTOCORRECTION_OCCURED:
    case ui::AX_EVENT_BLUR:
    case ui::AX_EVENT_CHECKED_STATE_CHANGED:
    case ui::AX_EVENT_CHILDREN_CHANGED:
    case ui::AX_EVENT_CLICKED:
    case ui::AX_EVENT_DOCUMENT_SELECTION_CHANGED:
    case ui::AX_EVENT_EXPANDED_CHANGED:
    case ui::AX_EVENT_FOCUS:
    case ui::AX_EVENT_HIDE:
    case ui::AX_EVENT_HOVER:
    case ui::AX_EVENT_IMAGE_FRAME_UPDATED:
    case ui::AX_EVENT_INVALID_STATUS_CHANGED:
    case ui::AX_EVENT_LAYOUT_COMPLETE:
    case ui::AX_EVENT_LIVE_REGION_CREATED:
    case ui::AX_EVENT_LIVE_REGION_CHANGED:
    case ui::AX_EVENT_LOCATION_CHANGED:
    case ui::AX_EVENT_MEDIA_STARTED_PLAYING:
    case ui::AX_EVENT_MEDIA_STOPPED_PLAYING:
    case ui::AX_EVENT_MENU_END:
    case ui::AX_EVENT_MENU_LIST_ITEM_SELECTED:
    case ui::AX_EVENT_MENU_LIST_VALUE_CHANGED:
    case ui::AX_EVENT_MENU_POPUP_END:
    case ui::AX_EVENT_MENU_POPUP_START:
    case ui::AX_EVENT_MENU_START:
    case ui::AX_EVENT_MOUSE_CANCELED:
    case ui::AX_EVENT_MOUSE_DRAGGED:
    case ui::AX_EVENT_MOUSE_MOVED:
    case ui::AX_EVENT_MOUSE_PRESSED:
    case ui::AX_EVENT_MOUSE_RELEASED:
    case ui::AX_EVENT_ROW_COLLAPSED:
    case ui::AX_EVENT_ROW_COUNT_CHANGED:
    case ui::AX_EVENT_ROW_EXPANDED:
    case ui::AX_EVENT_SCROLL_POSITION_CHANGED:
    case ui::AX_EVENT_SCROLLED_TO_ANCHOR:
    case ui::AX_EVENT_SELECTED_CHILDREN_CHANGED:
    case ui::AX_EVENT_SELECTION:
    case ui::AX_EVENT_SELECTION_ADD:
    case ui::AX_EVENT_SELECTION_REMOVE:
    case ui::AX_EVENT_SHOW:
    case ui::AX_EVENT_TEXT_CHANGED:
    case ui::AX_EVENT_TEXT_SELECTION_CHANGED:
    case ui::AX_EVENT_TREE_CHANGED:
    case ui::AX_EVENT_VALUE_CHANGED:
      return true;

    case ui::AX_EVENT_LOAD_COMPLETE:
      return false;

    case ui::AX_EVENT_NONE:
      return false;
  }

  NOTREACHED();
  return false;
}

}  // namespace extensions
