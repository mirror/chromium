// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/html/ListItemOrdinal.h"

#include "core/dom/LayoutTreeBuilderTraversal.h"
#include "core/html/HTMLOListElement.h"
#include "core/layout/LayoutListItem.h"
#include "core/layout/api/LayoutLIItem.h"

namespace blink {

bool ListItemOrdinal::IsList(const Node& node) {
  return isHTMLUListElement(node) || isHTMLOListElement(node);
}

bool ListItemOrdinal::IsListItem(const LayoutObject* layout_object) {
  return layout_object && layout_object->IsListItem();
}

bool ListItemOrdinal::IsListItem(const Node& node) {
  return IsListItem(node.GetLayoutObject());
}

ListItemOrdinal* ListItemOrdinal::Get(const Node& item_node) {
  LayoutObject* layout_object = item_node.GetLayoutObject();
  if (!layout_object)
    return nullptr;
  if (layout_object->IsListItem())
    return &ToLayoutListItem(layout_object)->Ordinal();
  return nullptr;
}

// Returns the enclosing list with respect to the DOM order.
Node* ListItemOrdinal::EnclosingList(const Node* list_item_node) {
  if (!list_item_node)
    return nullptr;
  Node* first_node = nullptr;
  // We use parentNode because the enclosing list could be a ShadowRoot that's
  // not Element.
  for (Node* parent = FlatTreeTraversal::Parent(*list_item_node); parent;
       parent = FlatTreeTraversal::Parent(*parent)) {
    if (IsList(*parent))
      return parent;
    if (!first_node)
      first_node = parent;
  }

  // If there's no actual <ul> or <ol> list element, then the first found
  // node acts as our list for purposes of determining what other list items
  // should be numbered as part of the same list.
  return first_node;
}

// Returns the next list item with respect to the DOM order.
ListItemOrdinal::NodeAndOrdinal ListItemOrdinal::NextListItem(const Node* list_node, const Node* item) {
  if (!list_node)
    return {};

  const Node* current = item ? item : list_node;
  DCHECK(current);
  DCHECK(!current->GetDocument().ChildNeedsDistributionRecalc());
  current = LayoutTreeBuilderTraversal::Next(*current, list_node);

  while (current) {
    if (IsList(*current)) {
      // We've found a nested, independent list: nothing to do here.
      current =
          LayoutTreeBuilderTraversal::NextSkippingChildren(*current, list_node);
      continue;
    }

    if (ListItemOrdinal* ordinal = Get(*current))
      return {current, ordinal};

    // FIXME: Can this be optimized to skip the children of the elements without
    // a layoutObject?
    current = LayoutTreeBuilderTraversal::Next(*current, list_node);
  }

  return {};
}

// Returns the previous list item with respect to the DOM order.
ListItemOrdinal::NodeAndOrdinal ListItemOrdinal::PreviousListItem(const Node* list_node, const Node* item) {
  const Node* current = item;
  DCHECK(current);
  DCHECK(!current->GetDocument().ChildNeedsDistributionRecalc());
  for (current = LayoutTreeBuilderTraversal::Previous(*current, list_node);
       current && current != list_node;
       current = LayoutTreeBuilderTraversal::Previous(*current, list_node)) {
    ListItemOrdinal* ordinal = Get(*current);
    if (!ordinal)
      continue;
    const Node* other_list = EnclosingList(current);
    // This item is part of our current list, so it's what we're looking for.
    if (list_node == other_list)
      return {current, ordinal};
    // We found ourself inside another list; lets skip the rest of it.
    // Use nextIncludingPseudo() here because the other list itself may actually
    // be a list item itself. We need to examine it, so we do this to counteract
    // the previousIncludingPseudo() that will be done by the loop.
    if (other_list)
      current = LayoutTreeBuilderTraversal::Next(*other_list, list_node);
  }
  return {};
}

ListItemOrdinal::NodeAndOrdinal ListItemOrdinal::NextOrdinalItem(
    bool is_list_reversed,
    const Node* list,
    const Node* item) {
  return is_list_reversed ? PreviousListItem(list, item)
                          : NextListItem(list, item);
}

Optional<int> ListItemOrdinal::ExplicitValue() const {
  if (!HasExplicitValue())
    return {};
  return value_;
}

int ListItemOrdinal::CalcValue(const Node& item_node) const {
  if (HasExplicitValue())
    return value_;

  Node* list = EnclosingList(&item_node);
  HTMLOListElement* o_list_element =
      isHTMLOListElement(list) ? toHTMLOListElement(list) : nullptr;
  int value_step = 1;
  if (o_list_element && o_list_element->IsReversed())
    value_step = -1;

  // FIXME: This recurses to a possible depth of the length of the list.
  // That's not good -- we need to change this to an iterative algorithm.
  if (NodeAndOrdinal previous = PreviousListItem(list, &item_node))
    return ClampAdd(previous.ordinal->Value(*previous.node), value_step);

  if (o_list_element)
    return o_list_element->StartConsideringItemCount();

  return 1;
}

int ListItemOrdinal::Value(const Node& item_node) const {
  if (type_ != kDirty)
    return value_;
  value_ = CalcValue(item_node);
  type_ = kUpdated;
  return value_;
}

void ListItemOrdinal::ValueChanged(const Node* list_node, const Node* item_node) {
  bool is_list_reversed = isHTMLOListElement(list_node)
                              ? toHTMLOListElement(list_node)->IsReversed()
                              : false;
  NodeAndOrdinal item{item_node};
  for (;;) {
    LayoutObject* layout_object = item.node->GetLayoutObject();
    if (IsListItem(layout_object))
      ToLayoutListItem(layout_object)->OrdinalValueChanged();

    item = NextOrdinalItem(is_list_reversed, list_node, item.node);
    if (!item)
      return;

    DCHECK(item.ordinal);
    if (item.ordinal->type_ != kUpdated)
      return;
    item.ordinal->type_ = kDirty;
  }
}

void ListItemOrdinal::ValueChanged(const Node* item_node) {
  ValueChanged(EnclosingList(item_node), item_node);
}

void ListItemOrdinal::SetExplicitValue(int value, const Node& item_node) {
  if (HasExplicitValue() && value_ == value)
    return;
  value_ = value;
  type_ = kExplicit;
  ValueChanged(&item_node);
}

void ListItemOrdinal::ClearExplicitValue(const Node& item_node) {
  if (!HasExplicitValue())
    return;
  type_ = kDirty;
  ValueChanged(&item_node);
}

void ListItemOrdinal::SetNotInList(bool not_in_list) {
  not_in_list_ = not_in_list;
}

void ListItemOrdinal::UpdateItemValuesForOrderedList(
    const HTMLOListElement* list_node) {
  DCHECK(list_node);

  if (NodeAndOrdinal list_item = NextListItem(list_node))
    ValueChanged(list_node, list_item.node);
}

unsigned ListItemOrdinal::ItemCountForOrderedList(
    const HTMLOListElement* list_node) {
  DCHECK(list_node);

  unsigned item_count = 0;
  for (NodeAndOrdinal list_item = NextListItem(list_node); list_item;
       list_item = NextListItem(list_node, list_item.node))
    item_count++;

  return item_count;
}

void ListItemOrdinal::UpdateListMarkerNumbers(const LayoutListItem* layout_list_item) {
  // If distribution recalc is needed, updateListMarkerNumber will be re-invoked
  // after distribution is calculated.
  const Node* item_node = layout_list_item->GetNode();
  if (item_node->GetDocument().ChildNeedsDistributionRecalc())
    return;

#if 1
  ValueChanged(item_node);
#else
  Node* list_node = EnclosingList(item_node);
  CHECK(list_node);

  bool is_list_reversed = false;
  HTMLOListElement* o_list_element =
      isHTMLOListElement(list_node) ? toHTMLOListElement(list_node) : 0;
  if (o_list_element) {
    o_list_element->ItemCountChanged();
    is_list_reversed = o_list_element->IsReversed();
  }

  // FIXME: The n^2 protection below doesn't help if the elements were inserted
  // after the the list had already been displayed.

  // Avoid an O(n^2) walk over the children below when they're all known to be
  // attaching.
  if (list_node->NeedsAttach())
    return;

  for (NodeAndOrdinal item =
           NextOrdinalItem(is_list_reversed, list_node, item_node);
       item; item = NextOrdinalItem(is_list_reversed, list_node, item.node)) {
    if (item.ordinal->type_ != kUpdated) {
      // If an item has been marked for update before, we can safely
      // assume that all the following ones have too.
      // This gives us the opportunity to stop here and avoid
      // marking the same nodes again.
      break;
    }
    item->UpdateValue();
  }
#endif
}

}  // namespace blink
