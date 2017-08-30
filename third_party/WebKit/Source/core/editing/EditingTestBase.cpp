// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/editing/EditingTestBase.h"

#include "core/dom/Attr.h"
#include "core/dom/Document.h"
#include "core/dom/Element.h"
#include "core/dom/Range.h"
#include "core/dom/Text.h"
#include "core/frame/LocalFrameView.h"
#include "core/html/HTMLElement.h"
#include "core/testing/DummyPageHolder.h"
#include "platform/wtf/text/StringBuilder.h"

namespace blink {

namespace {

// Parse selection text notation into Selection object.
template <typename Strategy>
class SelectionParser final {
 public:
  SelectionParser() = default;
  ~SelectionParser() = default;

  SelectionTemplate<Strategy> Parse(Node* node) {
    Traverse(node);
    if (anchor_node_ && focus_node_) {
      return typename SelectionTemplate<Strategy>::Builder()
          .Collapse(PositionTemplate<Strategy>(anchor_node_, anchor_offset_))
          .Extend(PositionTemplate<Strategy>(focus_node_, focus_offset_))
          .Build();
    }
    DCHECK(focus_node_) << "Need just '|', or '^' and '|'";
    return typename SelectionTemplate<Strategy>::Builder()
        .Collapse(PositionTemplate<Strategy>(focus_node_, focus_offset_))
        .Build();
  }

 private:
  void HandleCharacterData(CharacterData* node) {
    int anchor_offset = -1;
    int focus_offset = -1;
    StringBuilder builder;
    for (unsigned i = 0; i < node->length(); ++i) {
      const UChar char_code = node->data()[i];
      if (char_code == '^') {
        DCHECK_EQ(anchor_offset, -1) << node->data();
        anchor_offset = i;
        continue;
      }
      if (char_code == '|') {
        DCHECK_EQ(focus_offset, -1) << node->data();
        focus_offset = i;
        continue;
      }
      builder.Append(char_code);
    }
    if (anchor_offset == -1 && focus_offset == -1)
      return;
    node->setData(builder.ToString());
    if (node->length() == 0) {
      ContainerNode* const parent_node = node->parentNode();
      DCHECK(parent_node) << node;
      const int offset = node->NodeIndex();
      if (anchor_offset >= 0)
        RememberSelectionAnchor(parent_node, offset);
      if (focus_offset >= 0)
        RememberSelectionFocus(parent_node, offset);
      parent_node->removeChild(node);
      return;
    }
    if (anchor_offset >= 0 && focus_offset >= 0) {
      if (anchor_offset > focus_offset) {
        RememberSelectionAnchor(node, anchor_offset - 1);
        RememberSelectionAnchor(node, focus_offset);
        return;
      }
      RememberSelectionAnchor(node, anchor_offset);
      RememberSelectionFocus(node, focus_offset - 1);
    }
    if (anchor_offset >= 0) {
      RememberSelectionAnchor(node, anchor_offset);
      return;
    }
    if (focus_offset < 0)
      return;
    RememberSelectionFocus(node, focus_offset);
  }

  void HandleElementNode(Element* element) {
    for (Node& child : Strategy::ChildrenOf(*element))
      Traverse(&child);
  }

  void RememberSelectionAnchor(Node* node, int offset) {
    DCHECK_EQ(anchor_node_, nullptr) << *anchor_node_;
    anchor_node_ = node;
    anchor_offset_ = offset;
  }

  void RememberSelectionFocus(Node* node, int offset) {
    DCHECK_EQ(focus_node_, nullptr) << *focus_node_;
    focus_node_ = node;
    focus_offset_ = offset;
  }

  void Traverse(Node* node) {
    if (node->IsElementNode()) {
      HandleElementNode(ToElement(node));
      return;
    }
    if (node->IsCharacterDataNode()) {
      HandleCharacterData(ToCharacterData(node));
      return;
    }
    NOTREACHED() << node;
  }

  Node* anchor_node_ = nullptr;
  Node* focus_node_ = nullptr;
  int anchor_offset_ = 0;
  int focus_offset_ = 0;
};

// Serialize DOM/Flat tree to selection text.
template <typename Strategy>
class Serializer final {
 public:
  explicit Serializer(const SelectionTemplate<Strategy>& selection)
      : selection_(selection) {}

  std::string Serialize(const Node& root) {
    HandleNode(root);
    return builder_.ToString().Utf8().data();
  }

 private:
  void HandleCharacterData(const CharacterData& node) {
    const String text = node.data();
    if (selection_.IsNone()) {
      builder_.Append(text);
      return;
    }
    const Node& base_node = *selection_.Base().ComputeContainerNode();
    const Node& extent_node = *selection_.Extent().ComputeContainerNode();
    const int base_offset = selection_.Base().ComputeOffsetInContainerNode();
    const int extent_offset =
        selection_.Extent().ComputeOffsetInContainerNode();
    if (base_node == node && extent_node == node) {
      if (base_offset == extent_offset) {
        builder_.Append(text.Left(base_offset));
        builder_.Append('|');
        builder_.Append(text.Substring(base_offset));
      }
      if (base_offset < extent_offset) {
        builder_.Append(text.Left(base_offset));
        builder_.Append('^');
        builder_.Append(
            text.Substring(base_offset, extent_offset - base_offset));
        builder_.Append('|');
        builder_.Append(text.Substring(extent_offset));
      }
      builder_.Append(text.Left(extent_offset));
      builder_.Append('|');
      builder_.Append(
          text.Substring(extent_offset, base_offset - extent_offset));
      builder_.Append('^');
      builder_.Append(text.Substring(base_offset));
      return;
    }
    if (base_node == node) {
      builder_.Append(text.Left(base_offset));
      builder_.Append('^');
      builder_.Append(text.Substring(base_offset));
      return;
    }
    if (extent_node == node) {
      builder_.Append(text.Left(extent_offset));
      builder_.Append('|');
      builder_.Append(text.Substring(extent_offset));
      return;
    }
    builder_.Append(text);
  }

  void HandleElementNode(const Element& element) {
    const String tagName = element.tagName();
    builder_.Append('<');
    builder_.Append(element.tagName());
    if (AttrNodeList* attrs = const_cast<Element&>(element).GetAttrNodeList()) {
      for (const Attr* attr : *attrs) {
        builder_.Append(' ');
        builder_.Append(attr->name());
        if (attr->value().IsEmpty())
          continue;
        builder_.Append("=\"");
        for (size_t i = 0; i < attr->value().length(); ++i) {
          const UChar char_code = attr->value()[i];
          if (char_code == '\'') {
            builder_.Append("&apos;");
            continue;
          }
          if (char_code == '&') {
            builder_.Append("&amp;");
            continue;
          }
          builder_.Append(char_code);
        }
        builder_.Append('"');
      }
    }
    builder_.Append('>');
    if (IsVoidElement(element))
      return;
    SerializeChildren(element);
    builder_.Append("</");
    builder_.Append(element.tagName());
    builder_.Append('>');
  }

  void HandleNode(const Node& node) {
    if (node.IsElementNode()) {
      HandleElementNode(ToElement(node));
      return;
    }
    if (node.IsCharacterDataNode()) {
      HandleCharacterData(ToCharacterData(node));
      return;
    }
    NOTREACHED() << node;
  }

  void HandleSelection(const PositionTemplate<Strategy>& passed_position) {
    if (selection_.IsNone())
      return;
    const PositionTemplate<Strategy> position =
        passed_position.ToOffsetInAnchor();
    if (selection_.Extent().ToOffsetInAnchor() == position) {
      builder_.Append('|');
      return;
    }
    if (selection_.Base().ToOffsetInAnchor() != position)
      return;
    builder_.Append('^');
  }

  static bool IsVoidElement(const Element& element) {
    if (Strategy::FirstChild(element))
      return false;
    const String tag_name = element.tagName();
    return tag_name == "area" || tag_name == "base" || tag_name == "br" ||
           tag_name == "command" || tag_name == "embed" || tag_name == "hr" ||
           tag_name == "img" || tag_name == "input" || tag_name == "link" ||
           tag_name == "meta" || tag_name == "param" || tag_name == "source" ||
           tag_name == "track" || tag_name == "wbr";
  }

  void SerializeChildren(const ContainerNode& container) {
    if (!Strategy::FirstChild(container)) {
      HandleSelection(
          PositionTemplate<Strategy>::FirstPositionInNode(container));
      return;
    }
    int offset = 0;
    for (const Node& child : Strategy::ChildrenOf(container)) {
      HandleSelection(PositionTemplate<Strategy>(child, offset));
      HandleNode(child);
      ++offset;
    }
    HandleSelection(PositionTemplate<Strategy>::LastPositionInNode(container));
  }

  StringBuilder builder_;
  SelectionTemplate<Strategy> selection_;
};

}  // namespace

EditingTestBase::EditingTestBase() {}

EditingTestBase::~EditingTestBase() {}

Document& EditingTestBase::GetDocument() const {
  return dummy_page_holder_->GetDocument();
}

LocalFrame& EditingTestBase::GetFrame() const {
  return GetDummyPageHolder().GetFrame();
}

FrameSelection& EditingTestBase::Selection() const {
  return GetFrame().Selection();
}

Position EditingTestBase::AsPosition(const std::string& html_text) {
  const SelectionInDOMTree selection = AsSelection(html_text);
  DCHECK(selection.IsCaret());
  return selection.Base();
}

SelectionInDOMTree EditingTestBase::AsSelection(const std::string& html_text) {
  SetBodyContent(html_text);
  return SelectionParser<EditingStrategy>().Parse(GetDocument().body());
}

std::string EditingTestBase::ToSelectionText(
    const Node& root,
    const SelectionInDOMTree& selection) const {
  return Serializer<EditingStrategy>(selection).Serialize(root);
}

void EditingTestBase::SetUp() {
  dummy_page_holder_ = DummyPageHolder::Create(IntSize(800, 600));
}

void EditingTestBase::SetupPageWithClients(Page::PageClients* clients) {
  DCHECK(!dummy_page_holder_) << "Page should be set up only once";
  dummy_page_holder_ = DummyPageHolder::Create(IntSize(800, 600), clients);
}

ShadowRoot* EditingTestBase::CreateShadowRootForElementWithIDAndSetInnerHTML(
    TreeScope& scope,
    const char* host_element_id,
    const char* shadow_root_content) {
  ShadowRoot* shadow_root =
      scope.getElementById(AtomicString::FromUTF8(host_element_id))
          ->CreateShadowRootInternal(ShadowRootType::V0, ASSERT_NO_EXCEPTION);
  shadow_root->setInnerHTML(String::FromUTF8(shadow_root_content),
                            ASSERT_NO_EXCEPTION);
  scope.GetDocument().View()->UpdateAllLifecyclePhases();
  return shadow_root;
}

void EditingTestBase::SetBodyContent(const std::string& body_content) {
  GetDocument().body()->setInnerHTML(String::FromUTF8(body_content.c_str()),
                                     ASSERT_NO_EXCEPTION);
  UpdateAllLifecyclePhases();
}

ShadowRoot* EditingTestBase::SetShadowContent(const char* shadow_content,
                                              const char* host) {
  ShadowRoot* shadow_root = CreateShadowRootForElementWithIDAndSetInnerHTML(
      GetDocument(), host, shadow_content);
  return shadow_root;
}

void EditingTestBase::UpdateAllLifecyclePhases() {
  GetDocument().View()->UpdateAllLifecyclePhases();
}

}  // namespace blink
