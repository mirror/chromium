// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/accessibility/ax_event_generator.h"

#include "ui/accessibility/ax_node.h"
#include "ui/accessibility/ax_role_properties.h"

namespace ui {

AXEventGenerator::TargetedEvent::TargetedEvent(ui::AXNode* node, Event event)
    : node(node), event(event) {}

AXEventGenerator::AXEventGenerator(AXTree* tree) : tree_(tree) {}

AXEventGenerator::~AXEventGenerator() {}

std::vector<AXEventGenerator::TargetedEvent>
AXEventGenerator::PopAllQueuedEvents() {
  std::vector<AXEventGenerator::TargetedEvent> result;

  for (auto& entry : tree_events_) {
    ui::AXNode* event_target = tree_->GetFromId(entry.first);
    if (!event_target)
      continue;

    std::set<Event>& events = entry.second;
    if (events.find(Event::LIVE_REGION_CREATED) != events.end() ||
        events.find(Event::ALERT) != events.end()) {
      events.erase(Event::LIVE_REGION_CHANGED);
    }
    for (auto event : events)
      result.push_back(TargetedEvent(event_target, event));
  }
  tree_events_.clear();

  return result;
}

void AXEventGenerator::PushEvent(ui::AXNode* node,
                                 AXEventGenerator::Event event) {
  tree_events_[node->id()].insert(event);
}

void AXEventGenerator::OnNodeDataWillChange(AXTree* tree,
                                            const AXNodeData& old_node_data,
                                            const AXNodeData& new_node_data) {
  DCHECK_EQ(tree_, tree);
  if (new_node_data.child_ids != old_node_data.child_ids &&
      new_node_data.role != ui::AX_ROLE_STATIC_TEXT) {
    tree_events_[new_node_data.id].insert(Event::CHILDREN_CHANGED);
  }
}

void AXEventGenerator::OnRoleChanged(AXTree* tree,
                                     AXNode* node,
                                     AXRole old_role,
                                     AXRole new_role) {
  DCHECK_EQ(tree_, tree);
  PushEvent(node, Event::ROLE_CHANGED);
}

void AXEventGenerator::OnStateChanged(AXTree* tree,
                                      AXNode* node,
                                      AXState state,
                                      bool new_value) {
  DCHECK_EQ(tree_, tree);

  PushEvent(node, Event::STATE_CHANGED);
  if (state == ui::AX_STATE_EXPANDED) {
    if (new_value)
      PushEvent(node, Event::EXPANDED);
    else
      PushEvent(node, Event::COLLAPSED);
    if (node->data().role == ui::AX_ROLE_ROW ||
        node->data().role == ui::AX_ROLE_TREE_ITEM) {
      ui::AXNode* container = node;
      while (container && !ui::IsRowContainer(container->data().role))
        container = container->parent();
      if (container)
        PushEvent(container, Event::ROW_COUNT_CHANGED);
    }
  }
  if (state == ui::AX_STATE_SELECTED) {
    PushEvent(node, Event::SELECTED_CHANGED);
    ui::AXNode* container = node;
    while (container &&
           !ui::IsContainerWithSelectableChildrenRole(container->data().role))
      container = container->parent();
    if (container)
      PushEvent(container, Event::SELECTED_CHILDREN_CHANGED);
  }
}

void AXEventGenerator::OnStringAttributeChanged(AXTree* tree,
                                                AXNode* node,
                                                AXStringAttribute attr,
                                                const std::string& old_value,
                                                const std::string& new_value) {
  DCHECK_EQ(tree_, tree);

  switch (attr) {
    case ui::AX_ATTR_NAME:
      PushEvent(node, Event::NAME_CHANGED);
      break;
    case ui::AX_ATTR_DESCRIPTION:
      PushEvent(node, Event::DESCRIPTION_CHANGED);
      break;
    case ui::AX_ATTR_VALUE:
      PushEvent(node, Event::VALUE_CHANGED);
      break;
    case ui::AX_ATTR_ARIA_INVALID_VALUE:
      PushEvent(node, Event::INVALID_STATUS_CHANGED);
      break;
    case ui::AX_ATTR_LIVE_STATUS:
      PushEvent(node, Event::LIVE_REGION_CREATED);
      break;
    default:
      PushEvent(node, Event::OTHER_ATTRIBUTE_CHANGED);
      break;
  }
}

void AXEventGenerator::OnIntAttributeChanged(AXTree* tree,
                                             AXNode* node,
                                             AXIntAttribute attr,
                                             int32_t old_value,
                                             int32_t new_value) {
  DCHECK_EQ(tree_, tree);

  switch (attr) {
    case ui::AX_ATTR_ACTIVEDESCENDANT_ID:
      PushEvent(node, Event::ACTIVE_DESCENDANT_CHANGED);
      break;
    case ui::AX_ATTR_CHECKED_STATE:
      PushEvent(node, Event::CHECKED_STATE_CHANGED);
      break;
    case ui::AX_ATTR_INVALID_STATE:
      PushEvent(node, Event::INVALID_STATUS_CHANGED);
      break;
    case ui::AX_ATTR_SCROLL_X:
    case ui::AX_ATTR_SCROLL_Y:
      PushEvent(node, Event::SCROLL_POSITION_CHANGED);
      break;
    default:
      PushEvent(node, Event::OTHER_ATTRIBUTE_CHANGED);
      break;
  }
}

void AXEventGenerator::OnFloatAttributeChanged(AXTree* tree,
                                               AXNode* node,
                                               AXFloatAttribute attr,
                                               float old_value,
                                               float new_value) {
  DCHECK_EQ(tree_, tree);

  if (attr == ui::AX_ATTR_VALUE_FOR_RANGE)
    PushEvent(node, Event::VALUE_CHANGED);
  else
    PushEvent(node, Event::OTHER_ATTRIBUTE_CHANGED);
}

void AXEventGenerator::OnBoolAttributeChanged(AXTree* tree,
                                              AXNode* node,
                                              AXBoolAttribute attr,
                                              bool new_value) {
  DCHECK_EQ(tree_, tree);

  PushEvent(node, Event::OTHER_ATTRIBUTE_CHANGED);
}

void AXEventGenerator::OnIntListAttributeChanged(
    AXTree* tree,
    AXNode* node,
    AXIntListAttribute attr,
    const std::vector<int32_t>& old_value,
    const std::vector<int32_t>& new_value) {
  DCHECK_EQ(tree_, tree);
  PushEvent(node, Event::OTHER_ATTRIBUTE_CHANGED);
}

void AXEventGenerator::OnStringListAttributeChanged(
    AXTree* tree,
    AXNode* node,
    AXStringListAttribute attr,
    const std::vector<std::string>& old_value,
    const std::vector<std::string>& new_value) {
  DCHECK_EQ(tree_, tree);
}

void AXEventGenerator::OnTreeDataChanged(AXTree* tree,
                                         const ui::AXTreeData& old_tree_data,
                                         const ui::AXTreeData& new_tree_data) {
  DCHECK_EQ(tree_, tree);

  if (new_tree_data.loaded && !old_tree_data.loaded)
    PushEvent(tree->root(), Event::LOAD_COMPLETE);
  if (new_tree_data.sel_anchor_object_id !=
          old_tree_data.sel_anchor_object_id ||
      new_tree_data.sel_anchor_offset != old_tree_data.sel_anchor_offset ||
      new_tree_data.sel_anchor_affinity != old_tree_data.sel_anchor_affinity ||
      new_tree_data.sel_focus_object_id != old_tree_data.sel_focus_object_id ||
      new_tree_data.sel_focus_offset != old_tree_data.sel_focus_offset ||
      new_tree_data.sel_focus_affinity != old_tree_data.sel_focus_affinity) {
    PushEvent(tree->root(), Event::DOCUMENT_SELECTION_CHANGED);
  }
}

void AXEventGenerator::OnNodeWillBeDeleted(AXTree* tree, AXNode* node) {
  DCHECK_EQ(tree_, tree);
}

void AXEventGenerator::OnSubtreeWillBeDeleted(AXTree* tree, AXNode* node) {
  DCHECK_EQ(tree_, tree);
}

void AXEventGenerator::OnNodeWillBeReparented(AXTree* tree, AXNode* node) {
  DCHECK_EQ(tree_, tree);
}

void AXEventGenerator::OnSubtreeWillBeReparented(AXTree* tree, AXNode* node) {
  DCHECK_EQ(tree_, tree);
}

void AXEventGenerator::OnNodeCreated(AXTree* tree, AXNode* node) {
  DCHECK_EQ(tree_, tree);
}

void AXEventGenerator::OnNodeReparented(AXTree* tree, AXNode* node) {
  DCHECK_EQ(tree_, tree);
}

void AXEventGenerator::OnNodeChanged(AXTree* tree, AXNode* node) {
  DCHECK_EQ(tree_, tree);
}

void AXEventGenerator::OnAtomicUpdateFinished(
    AXTree* tree,
    bool root_changed,
    const std::vector<Change>& changes) {
  DCHECK_EQ(tree_, tree);

  if (root_changed && tree->data().loaded)
    PushEvent(tree->root(), Event::LOAD_COMPLETE);

  for (const auto& change : changes) {
    if ((change.type == NODE_CREATED || change.type == SUBTREE_CREATED) &&
        change.node->data().HasStringAttribute(ui::AX_ATTR_LIVE_STATUS)) {
      if (change.node->data().role == ui::AX_ROLE_ALERT)
        PushEvent(change.node, Event::ALERT);
      else
        PushEvent(change.node, Event::LIVE_REGION_CREATED);
      continue;
    }
    if (change.node->data().HasStringAttribute(
            ui::AX_ATTR_CONTAINER_LIVE_STATUS)) {
      if (!change.node->data().GetStringAttribute(ui::AX_ATTR_NAME).empty())
        PushEvent(change.node, Event::LIVE_REGION_NODE_CHANGED);
      ui::AXNode* live_root = change.node;
      while (live_root &&
             !live_root->data().HasStringAttribute(ui::AX_ATTR_LIVE_STATUS))
        live_root = live_root->parent();
      if (live_root)
        PushEvent(live_root, Event::LIVE_REGION_CHANGED);
    }
  }
}

}  // namespace ui
