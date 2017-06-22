// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/dom/WhitespaceAttacher.h"

#include "core/dom/Element.h"
#include "core/dom/Text.h"
#include "core/html/HTMLSlotElement.h"
#include "core/layout/LayoutText.h"
#include "platform/ScriptForbiddenScope.h"

namespace blink {

WhitespaceAttacher::~WhitespaceAttacher() {
  if (last_text_node_ && last_text_node_needs_reattach_)
    ReattachWhitespaceSiblings(nullptr);
}

void WhitespaceAttacher::DidReattach(Node* node, LayoutObject* prev_in_flow) {
  DCHECK(node);
  DCHECK(node->IsTextNode() || node->IsElementNode());
  DCHECK(!last_display_contents_ || !last_text_node_needs_reattach_);

  ForceLastTextNodeNeedsReattach();

  // No subsequent text nodes affected.
  if (!last_text_node_)
    return;

  LayoutObject* layout_object = node->GetLayoutObject();
  if (!layout_object)
    layout_object = prev_in_flow;

  // If we re-attached a display:contents, we would not set the
  // last_display_contents_. If last_display_contents_ is set at this point, it
  // means the attached node did not contain any in-flow children.
  if (!layout_object)
    return;

  // Only in-flow boxes affect subsequent whitespace.
  if (!layout_object->IsFloatingOrOutOfFlowPositioned())
    ReattachWhitespaceSiblings(layout_object);
}

void WhitespaceAttacher::DidReattachText(Text* text) {
  DCHECK(text);
  DidReattach(text, text->GetLayoutObject());
  SetLastTextNode(text);
  if (!text->GetLayoutObject())
    last_text_node_needs_reattach_ = true;
}

void WhitespaceAttacher::DidReattachElement(Element* element,
                                            LayoutObject* prev_in_flow) {
  DCHECK(element);
  DidReattach(element, prev_in_flow);
}

void WhitespaceAttacher::DidVisitText(Text* text) {
  DCHECK(text);
  if (!last_text_node_ || !last_text_node_needs_reattach_) {
    SetLastTextNode(text);
    return;
  }
  DCHECK(!last_display_contents_);
  if (LayoutObject* text_layout_object = text->GetLayoutObject()) {
    ReattachWhitespaceSiblings(text_layout_object);
  } else {
    if (last_text_node_->ContainsOnlyWhitespace())
      last_text_node_->ReattachLayoutTreeIfNeeded(Node::AttachContext());
  }
  SetLastTextNode(text);
}

void WhitespaceAttacher::DidVisitElement(Element* element) {
  DCHECK(element);
  LayoutObject* layout_object = element->GetLayoutObject();
  if (!layout_object) {
    if (last_text_node_needs_reattach_)
      return;
    if (element->HasDisplayContentsStyle() ||
        element->IsActiveSlotOrActiveInsertionPoint())
      last_display_contents_ = element;
    return;
  }
  if (!last_text_node_ || !last_text_node_needs_reattach_) {
    SetLastTextNode(nullptr);
    return;
  }
  if (layout_object->IsFloatingOrOutOfFlowPositioned())
    return;
  ReattachWhitespaceSiblings(layout_object);
}

void WhitespaceAttacher::ReattachWhitespaceSiblings(
    LayoutObject* previous_in_flow) {
  DCHECK(!last_display_contents_);
  DCHECK(last_text_node_);
  DCHECK(last_text_node_needs_reattach_);
  ScriptForbiddenScope forbid_script;

  Node::AttachContext context;
  context.previous_in_flow = previous_in_flow;
  context.use_previous_in_flow = true;

  for (Node* sibling = last_text_node_; sibling;
       sibling = LayoutTreeBuilderTraversal::NextLayoutSibling(*sibling)) {
    LayoutObject* sibling_layout_object = sibling->GetLayoutObject();
    if (sibling->IsTextNode() && ToText(sibling)->ContainsOnlyWhitespace()) {
      bool had_layout_object = !!sibling_layout_object;
      ToText(sibling)->ReattachLayoutTreeIfNeeded(context);
      sibling_layout_object = sibling->GetLayoutObject();
      // If sibling's layout object status didn't change we don't need to
      // continue checking other siblings since their layout object status
      // won't change either.
      if (!!sibling_layout_object == had_layout_object)
        break;
      if (sibling_layout_object)
        context.previous_in_flow = sibling_layout_object;
    } else if (sibling_layout_object &&
               !sibling_layout_object->IsFloatingOrOutOfFlowPositioned()) {
      break;
    }
  }
  SetLastTextNode(nullptr);
}

void WhitespaceAttacher::ForceLastTextNodeNeedsReattach() {
  // If an element got re-attached, the need for a subsequent whitespace node
  // LayoutObject may have changed. Make sure we try a re-attach when we
  // encounter the next in-flow.
  if (last_text_node_needs_reattach_)
    return;
  if (last_display_contents_)
    UpdateLastTextNodeFromDisplayContents();
  if (last_text_node_)
    last_text_node_needs_reattach_ = true;
}

void WhitespaceAttacher::UpdateLastTextNodeFromDisplayContents() {
  DCHECK(last_display_contents_);

  // TODO(rune@opera.com): The slot-specific code below can be removed once we
  // treat slot elements as display:contents flat tree members as specified.
  Node* child;
  if (last_display_contents_->IsActiveSlotOrActiveInsertionPoint()) {
    if (last_display_contents_->IsInsertionPoint()) {
      child = ToInsertionPoint(last_display_contents_.Get())
                  ->FirstDistributedNode();
    } else {
      child = toHTMLSlotElement(last_display_contents_.Get())
                  ->FirstDistributedNode();
    }
  } else {
    child =
        LayoutTreeBuilderTraversal::FirstLayoutChild(*last_display_contents_);
  }

  last_display_contents_ = nullptr;

  // If the display:contents was empty, use the current last_text_node_.
  if (!child)
    return;

  for (; child && child != last_text_node_;
       child = LayoutTreeBuilderTraversal::NextLayoutSibling(*child)) {
    LayoutObject* layout_object = child->GetLayoutObject();
    if (child->IsTextNode()) {
      Text* text = ToText(child);
      if (text->ContainsOnlyWhitespace()) {
        last_text_node_ = text;
        return;
      }
    }
    if (layout_object && !layout_object->IsFloatingOrOutOfFlowPositioned()) {
      last_text_node_ = nullptr;
      break;
    }
  }
}

}  // namespace blink
