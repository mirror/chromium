// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/dom/WhitespaceAttacher.h"

#include "core/dom/Element.h"
#include "core/dom/Text.h"
#include "core/layout/LayoutText.h"
#include "platform/ScriptForbiddenScope.h"

namespace blink {

void WhitespaceAttacher::DidVisitText(Text* text) {
  DCHECK(text);
  if (last_text_node_) {
    LayoutObject* layout_object = text->GetLayoutObject();
    if (!layout_object)
      return;
    if (last_text_node_needs_reattach_) {
      ReattachWhitespaceSiblings(layout_object);
    }
  }
  SetLastTextNode(text);
}

void WhitespaceAttacher::DidReattachText(Text* text) {
  DCHECK(text);
  if (last_text_node_) {
    last_text_node_needs_reattach_ = true;
    if (LayoutObject* layout_object = text->GetLayoutObject())
      ReattachWhitespaceSiblings(layout_object);
  }
  SetLastTextNode(text);
}

void WhitespaceAttacher::DidVisitElement(const Element* element) {
  DCHECK(element);
  if (!last_text_node_)
    return;
  LayoutObject* layout_object = element->GetLayoutObject();
  if (!layout_object)
    return;
  if (layout_object->IsFloatingOrOutOfFlowPositioned())
    return;
  if (last_text_node_needs_reattach_)
    ReattachWhitespaceSiblings(layout_object);
  else
    SetLastTextNode(nullptr);
}

void WhitespaceAttacher::DidReattachElement(const Element* element) {
  DCHECK(element);
  if (!last_text_node_)
    return;
  last_text_node_needs_reattach_ = true;
  LayoutObject* layout_object = element->GetLayoutObject();
  if (!layout_object)
    return;
  if (layout_object->IsFloatingOrOutOfFlowPositioned())
    return;
  ReattachWhitespaceSiblings(layout_object);
}

void WhitespaceAttacher::FinishSiblings() {
  if (last_text_node_ && last_text_node_needs_reattach_)
    ReattachWhitespaceSiblings(nullptr);
  else
    SetLastTextNode(nullptr);
}

void WhitespaceAttacher::ReattachWhitespaceSiblings(
    LayoutObject* previous_in_flow) {
  DCHECK(last_text_node_);
  DCHECK(last_text_node_needs_reattach_);
  ScriptForbiddenScope forbid_script;

  Node::AttachContext context;
  context.previous_in_flow = previous_in_flow;
  context.use_previous_in_flow = true;

  for (Node* sibling = last_text_node_; sibling;
       sibling = sibling->nextSibling()) {
    if (sibling->IsTextNode() && ToText(sibling)->ContainsOnlyWhitespace()) {
      bool had_layout_object = !!sibling->GetLayoutObject();
      ToText(sibling)->ReattachLayoutTreeIfNeeded(context);
      LayoutObject* sibling_layout_object = sibling->GetLayoutObject();
      // If sibling's layout object status didn't change we don't need to
      // continue checking other siblings since their layout object status
      // won't change either.
      if (!!sibling_layout_object == had_layout_object)
        break;
      if (sibling_layout_object)
        context.previous_in_flow = sibling_layout_object;
    } else if (sibling->GetLayoutObject()) {
      break;
    }
  }
  SetLastTextNode(nullptr);
}

}  // namespace blink
