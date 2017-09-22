// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ListItemOrdinal_h
#define ListItemOrdinal_h

#include "platform/heap/Persistent.h"
#include "platform/wtf/Optional.h"

namespace blink {

class HTMLOListElement;
class LayoutListItem;
class LayoutObject;
class Node;

// Represents an "ordinal value" and its related algorithms as defined in the
// HTML spec:
// https://html.spec.whatwg.org/multipage/grouping-content.html#ordinal-value
class ListItemOrdinal {
 public:
  static ListItemOrdinal* Get(const Node&);

  int Value(const Node&) const;

  Optional<int> ExplicitValue() const;
  void SetExplicitValue(int, const Node&);
  void ClearExplicitValue(const Node&);

  bool NotInList() const { return not_in_list_; }
  void SetNotInList(bool);

  static bool IsList(const Node&);
  static bool IsListItem(const Node&);
  static bool IsListItem(const LayoutObject*);

  static void UpdateItemValuesForOrderedList(const HTMLOListElement*);
  static unsigned ItemCountForOrderedList(const HTMLOListElement*);
  static void UpdateListMarkerNumbers(const LayoutListItem*);

 private:
  static void Invalidate(const Node* list_node, const Node* item_node, ListItemOrdinal*);
  static void ValueChanged(const Node*);
  static void ValueChanged(const Node* list_node, const Node* item_node);

  bool HasExplicitValue() const { return type_ == kExplicit; }
  int CalcValue(const Node&) const;
  void ExplicitValueChanged();

  static Node* EnclosingList(const Node*);
  struct NodeAndOrdinal {
    STACK_ALLOCATED();
    Persistent<const Node> node;
    ListItemOrdinal* ordinal = nullptr;
    operator bool() const { return node; }
  };
  static NodeAndOrdinal NextListItem(const Node* list_node, const Node* item_node = nullptr);
  static NodeAndOrdinal PreviousListItem(const Node* list_node, const Node* item_node);
  static NodeAndOrdinal NextOrdinalItem(bool is_reversed, const Node* list_node, const Node* item_node = nullptr);

  mutable int value_ = 0;
  enum ValueType { kDirty, kUpdated, kExplicit };
  mutable ValueType type_ = kDirty;
  bool not_in_list_ : 1;
};

}  // namespace blink

#endif  // ListItemOrdinal_h
