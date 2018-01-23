// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/extensions/chrome_extension_messages.h"
#include "chrome/renderer/extensions/automation_internal_custom_bindings.h"
#include "ui/accessibility/ax_node.h"

namespace extensions {

namespace {

// Convert from ax::mojom::Event to api::automation::EventType.
api::automation::EventType ToAutomationEvent(ax::mojom::Event event_type) {
  switch (event_type) {
    case ax::mojom::Event::NONE:
      return api::automation::EVENT_TYPE_NONE;
    case ax::mojom::Event::ACTIVEDESCENDANTCHANGED:
      return api::automation::EVENT_TYPE_ACTIVEDESCENDANTCHANGED;
    case ax::mojom::Event::ALERT:
      return api::automation::EVENT_TYPE_ALERT;
    case ax::mojom::Event::ARIA_ATTRIBUTE_CHANGED:
      return api::automation::EVENT_TYPE_ARIAATTRIBUTECHANGED;
    case ax::mojom::Event::AUTOCORRECTION_OCCURED:
      return api::automation::EVENT_TYPE_AUTOCORRECTIONOCCURED;
    case ax::mojom::Event::BLUR:
      return api::automation::EVENT_TYPE_BLUR;
    case ax::mojom::Event::CHECKED_STATE_CHANGED:
      return api::automation::EVENT_TYPE_CHECKEDSTATECHANGED;
    case ax::mojom::Event::CHILDREN_CHANGED:
      return api::automation::EVENT_TYPE_CHILDRENCHANGED;
    case ax::mojom::Event::CLICKED:
      return api::automation::EVENT_TYPE_CLICKED;
    case ax::mojom::Event::DOCUMENT_SELECTION_CHANGED:
      return api::automation::EVENT_TYPE_DOCUMENTSELECTIONCHANGED;
    case ax::mojom::Event::EXPANDED_CHANGED:
      return api::automation::EVENT_TYPE_EXPANDEDCHANGED;
    case ax::mojom::Event::FOCUS:
      return api::automation::EVENT_TYPE_FOCUS;
    case ax::mojom::Event::HIDE:
      return api::automation::EVENT_TYPE_HIDE;
    case ax::mojom::Event::HIT_TEST_RESULT:
      return api::automation::EVENT_TYPE_HITTESTRESULT;
    case ax::mojom::Event::HOVER:
      return api::automation::EVENT_TYPE_HOVER;
    case ax::mojom::Event::IMAGE_FRAME_UPDATED:
      return api::automation::EVENT_TYPE_IMAGEFRAMEUPDATED;
    case ax::mojom::Event::INVALID_STATUS_CHANGED:
      return api::automation::EVENT_TYPE_INVALIDSTATUSCHANGED;
    case ax::mojom::Event::LAYOUT_COMPLETE:
      return api::automation::EVENT_TYPE_LAYOUTCOMPLETE;
    case ax::mojom::Event::LIVE_REGION_CREATED:
      return api::automation::EVENT_TYPE_LIVEREGIONCREATED;
    case ax::mojom::Event::LIVE_REGION_CHANGED:
      return api::automation::EVENT_TYPE_LIVEREGIONCHANGED;
    case ax::mojom::Event::LOAD_COMPLETE:
      return api::automation::EVENT_TYPE_LOADCOMPLETE;
    case ax::mojom::Event::LOCATION_CHANGED:
      return api::automation::EVENT_TYPE_LOCATIONCHANGED;
    case ax::mojom::Event::MEDIA_STARTED_PLAYING:
      return api::automation::EVENT_TYPE_MEDIASTARTEDPLAYING;
    case ax::mojom::Event::MEDIA_STOPPED_PLAYING:
      return api::automation::EVENT_TYPE_MEDIASTOPPEDPLAYING;
    case ax::mojom::Event::MENU_END:
      return api::automation::EVENT_TYPE_MENUEND;
    case ax::mojom::Event::MENU_LIST_ITEM_SELECTED:
      return api::automation::EVENT_TYPE_MENULISTITEMSELECTED;
    case ax::mojom::Event::MENU_LIST_VALUE_CHANGED:
      return api::automation::EVENT_TYPE_MENULISTVALUECHANGED;
    case ax::mojom::Event::MENU_POPUP_END:
      return api::automation::EVENT_TYPE_MENUPOPUPEND;
    case ax::mojom::Event::MENU_POPUP_START:
      return api::automation::EVENT_TYPE_MENUPOPUPSTART;
    case ax::mojom::Event::MENU_START:
      return api::automation::EVENT_TYPE_MENUSTART;
    case ax::mojom::Event::MOUSE_CANCELED:
      return api::automation::EVENT_TYPE_MOUSECANCELED;
    case ax::mojom::Event::MOUSE_DRAGGED:
      return api::automation::EVENT_TYPE_MOUSEDRAGGED;
    case ax::mojom::Event::MOUSE_MOVED_VALUE:
      return api::automation::EVENT_TYPE_MOUSEMOVED;
    case ax::mojom::Event::MOUSE_PRESSED:
      return api::automation::EVENT_TYPE_MOUSEPRESSED;
    case ax::mojom::Event::MOUSE_RELEASED:
      return api::automation::EVENT_TYPE_MOUSERELEASED;
    case ax::mojom::Event::ROW_COLLAPSED:
      return api::automation::EVENT_TYPE_ROWCOLLAPSED;
    case ax::mojom::Event::ROW_COUNT_CHANGED:
      return api::automation::EVENT_TYPE_ROWCOUNTCHANGED;
    case ax::mojom::Event::ROW_EXPANDED:
      return api::automation::EVENT_TYPE_ROWEXPANDED;
    case ax::mojom::Event::SCROLL_POSITION_CHANGED:
      return api::automation::EVENT_TYPE_SCROLLPOSITIONCHANGED;
    case ax::mojom::Event::SCROLLED_TO_ANCHOR:
      return api::automation::EVENT_TYPE_SCROLLEDTOANCHOR;
    case ax::mojom::Event::SELECTED_CHILDREN_CHANGED:
      return api::automation::EVENT_TYPE_SELECTEDCHILDRENCHANGED;
    case ax::mojom::Event::SELECTION:
      return api::automation::EVENT_TYPE_SELECTION;
    case ax::mojom::Event::SELECTION_ADD:
      return api::automation::EVENT_TYPE_SELECTIONADD;
    case ax::mojom::Event::SELECTION_REMOVE:
      return api::automation::EVENT_TYPE_SELECTIONREMOVE;
    case ax::mojom::Event::SHOW:
      return api::automation::EVENT_TYPE_SHOW;
    case ax::mojom::Event::TEXT_CHANGED:
      return api::automation::EVENT_TYPE_TEXTCHANGED;
    case ax::mojom::Event::TEXT_SELECTION_CHANGED:
      return api::automation::EVENT_TYPE_TEXTSELECTIONCHANGED;
    case ax::mojom::Event::TREE_CHANGED:
      return api::automation::EVENT_TYPE_TREECHANGED;
    case ax::mojom::Event::VALUE_CHANGED:
      return api::automation::EVENT_TYPE_VALUECHANGED;
  }

  NOTREACHED();
  return api::automation::EVENT_TYPE_NONE;
}

// Convert from ui::AXEventGenerator::Event to api::automation::EventType.
api::automation::EventType ToAutomationEvent(
    ui::AXEventGenerator::Event event_type) {
  switch (event_type) {
    case ui::AXEventGenerator::Event::ACTIVE_DESCENDANT_CHANGED:
      return api::automation::EVENT_TYPE_ACTIVEDESCENDANTCHANGED;
    case ui::AXEventGenerator::Event::ALERT:
      return api::automation::EVENT_TYPE_ALERT;
    case ui::AXEventGenerator::Event::CHECKED_STATE_CHANGED:
      return api::automation::EVENT_TYPE_CHECKEDSTATECHANGED;
    case ui::AXEventGenerator::Event::CHILDREN_CHANGED:
      return api::automation::EVENT_TYPE_CHILDRENCHANGED;
    case ui::AXEventGenerator::Event::DOCUMENT_SELECTION_CHANGED:
      return api::automation::EVENT_TYPE_DOCUMENTSELECTIONCHANGED;
    case ui::AXEventGenerator::Event::INVALID_STATUS_CHANGED:
      return api::automation::EVENT_TYPE_INVALIDSTATUSCHANGED;
    case ui::AXEventGenerator::Event::LIVE_REGION_CHANGED:
      return api::automation::EVENT_TYPE_LIVEREGIONCHANGED;
    case ui::AXEventGenerator::Event::LIVE_REGION_CREATED:
      return api::automation::EVENT_TYPE_LIVEREGIONCREATED;
    case ui::AXEventGenerator::Event::LOAD_COMPLETE:
      return api::automation::EVENT_TYPE_LOADCOMPLETE;
    case ui::AXEventGenerator::Event::MENU_ITEM_SELECTED:
      return api::automation::EVENT_TYPE_MENULISTITEMSELECTED;
    case ui::AXEventGenerator::Event::ROW_COUNT_CHANGED:
      return api::automation::EVENT_TYPE_ROWCOUNTCHANGED;
    case ui::AXEventGenerator::Event::SCROLL_POSITION_CHANGED:
      return api::automation::EVENT_TYPE_SCROLLPOSITIONCHANGED;
    case ui::AXEventGenerator::Event::SELECTED_CHILDREN_CHANGED:
      return api::automation::EVENT_TYPE_SELECTEDCHILDRENCHANGED;
    case ui::AXEventGenerator::Event::VALUE_CHANGED:
      return api::automation::EVENT_TYPE_VALUECHANGED;

    // These don't have a mapping.
    case ui::AXEventGenerator::Event::COLLAPSED:
    case ui::AXEventGenerator::Event::DESCRIPTION_CHANGED:
    case ui::AXEventGenerator::Event::DOCUMENT_TITLE_CHANGED:
    case ui::AXEventGenerator::Event::EXPANDED:
    case ui::AXEventGenerator::Event::LIVE_REGION_NODE_CHANGED:
    case ui::AXEventGenerator::Event::NAME_CHANGED:
    case ui::AXEventGenerator::Event::OTHER_ATTRIBUTE_CHANGED:
    case ui::AXEventGenerator::Event::ROLE_CHANGED:
    case ui::AXEventGenerator::Event::SELECTED_CHANGED:
    case ui::AXEventGenerator::Event::STATE_CHANGED:
      return api::automation::EVENT_TYPE_NONE;
  }

  NOTREACHED();
  return api::automation::EVENT_TYPE_NONE;
}

}  // namespace

AutomationAXTreeWrapper::AutomationAXTreeWrapper(
    int32_t tree_id,
    AutomationInternalCustomBindings* owner)
    : tree_id_(tree_id), host_node_id_(-1), owner_(owner) {
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

  if (!tree_.Unserialize(params.update))
    return false;

  // Don't send any events if it's not the active profile.
  if (!is_active_profile)
    return true;

  owner_->SendNodesRemovedEvent(&tree_, deleted_node_ids_);
  deleted_node_ids_.clear();

  api::automation::EventType automation_event_type =
      ToAutomationEvent(params.event_type);

  // Send some events directly from the event message, if they're not
  // handled by AXEventGenerator yet.
  if (!IsEventTypeHandledByAXEventGenerator(automation_event_type))
    owner_->SendAutomationEvent(params, params.id, automation_event_type);

  // Send auto-generated AXEventGenerator events.
  for (auto targeted_event : *this) {
    api::automation::EventType event_type =
        ToAutomationEvent(targeted_event.event);
    if (IsEventTypeHandledByAXEventGenerator(event_type)) {
      owner_->SendAutomationEvent(params, targeted_event.node->id(),
                                  event_type);
    }
  }
  ClearEvents();

  return true;
}

void AutomationAXTreeWrapper::OnNodeDataWillChange(
    ui::AXTree* tree,
    const ui::AXNodeData& old_node_data,
    const ui::AXNodeData& new_node_data) {
  AXEventGenerator::OnNodeDataWillChange(tree, old_node_data, new_node_data);
  if (old_node_data.GetStringAttribute(ax::mojom::StringAttribute::NAME) !=
      new_node_data.GetStringAttribute(ax::mojom::StringAttribute::NAME))
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

bool AutomationAXTreeWrapper::IsEventTypeHandledByAXEventGenerator(
    api::automation::EventType event_type) const {
  switch (event_type) {
    // Generated by AXEventGenerator.
    case api::automation::EVENT_TYPE_ACTIVEDESCENDANTCHANGED:
    case api::automation::EVENT_TYPE_CHECKEDSTATECHANGED:
    case api::automation::EVENT_TYPE_EXPANDEDCHANGED:
    case api::automation::EVENT_TYPE_INVALIDSTATUSCHANGED:
    case api::automation::EVENT_TYPE_LIVEREGIONCHANGED:
    case api::automation::EVENT_TYPE_LIVEREGIONCREATED:
    case api::automation::EVENT_TYPE_LOADCOMPLETE:
    case api::automation::EVENT_TYPE_SCROLLPOSITIONCHANGED:
    case api::automation::EVENT_TYPE_SELECTEDCHILDRENCHANGED:
      return true;

    // Not generated by AXEventGenerator and possible candidates
    // for removal from the automation API entirely.
    case api::automation::EVENT_TYPE_HIDE:
    case api::automation::EVENT_TYPE_LAYOUTCOMPLETE:
    case api::automation::EVENT_TYPE_MENULISTVALUECHANGED:
    case api::automation::EVENT_TYPE_MENUPOPUPEND:
    case api::automation::EVENT_TYPE_MENUPOPUPSTART:
    case api::automation::EVENT_TYPE_SELECTIONADD:
    case api::automation::EVENT_TYPE_SELECTIONREMOVE:
    case api::automation::EVENT_TYPE_SHOW:
    case api::automation::EVENT_TYPE_TREECHANGED:
      return false;

    // These events will never be generated by AXEventGenerator.
    // These are all events that can't be inferred from a tree change.
    case api::automation::EVENT_TYPE_NONE:
    case api::automation::EVENT_TYPE_AUTOCORRECTIONOCCURED:
    case api::automation::EVENT_TYPE_CLICKED:
    case api::automation::EVENT_TYPE_HITTESTRESULT:
    case api::automation::EVENT_TYPE_HOVER:
    case api::automation::EVENT_TYPE_MEDIASTARTEDPLAYING:
    case api::automation::EVENT_TYPE_MEDIASTOPPEDPLAYING:
    case api::automation::EVENT_TYPE_MOUSECANCELED:
    case api::automation::EVENT_TYPE_MOUSEDRAGGED:
    case api::automation::EVENT_TYPE_MOUSEMOVED:
    case api::automation::EVENT_TYPE_MOUSEPRESSED:
    case api::automation::EVENT_TYPE_MOUSERELEASED:
    case api::automation::EVENT_TYPE_SCROLLEDTOANCHOR:
      return false;

    // These events might need to be migrated to AXEventGenerator.
    case api::automation::EVENT_TYPE_ALERT:
    case api::automation::EVENT_TYPE_ARIAATTRIBUTECHANGED:
    case api::automation::EVENT_TYPE_BLUR:
    case api::automation::EVENT_TYPE_CHILDRENCHANGED:
    case api::automation::EVENT_TYPE_DOCUMENTSELECTIONCHANGED:
    case api::automation::EVENT_TYPE_FOCUS:
    case api::automation::EVENT_TYPE_IMAGEFRAMEUPDATED:
    case api::automation::EVENT_TYPE_LOCATIONCHANGED:
    case api::automation::EVENT_TYPE_MENUEND:
    case api::automation::EVENT_TYPE_MENULISTITEMSELECTED:
    case api::automation::EVENT_TYPE_MENUSTART:
    case api::automation::EVENT_TYPE_ROWCOLLAPSED:
    case api::automation::EVENT_TYPE_ROWCOUNTCHANGED:
    case api::automation::EVENT_TYPE_ROWEXPANDED:
    case api::automation::EVENT_TYPE_SELECTION:
    case api::automation::EVENT_TYPE_TEXTCHANGED:
    case api::automation::EVENT_TYPE_TEXTSELECTIONCHANGED:
    case api::automation::EVENT_TYPE_VALUECHANGED:
      return false;
  }

  NOTREACHED();
  return false;
}

}  // namespace extensions
