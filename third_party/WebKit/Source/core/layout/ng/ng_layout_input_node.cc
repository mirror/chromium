// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/ng_layout_input_node.h"

#include "core/layout/LayoutReplaced.h"
#include "core/layout/ng/geometry/ng_intrinsic_size.h"
#include "core/layout/ng/inline/ng_inline_node.h"
#include "core/layout/ng/layout_ng_block_flow.h"
#include "core/layout/ng/ng_block_node.h"
#include "core/layout/ng/ng_constraint_space.h"
#include "core/layout/ng/ng_layout_result.h"
#include "core/layout/ng/ng_length_utils.h"
#include "core/layout/ng/ng_min_max_content_size.h"
#include "core/layout/ng/ng_unpositioned_float.h"
#include "platform/wtf/text/StringBuilder.h"

namespace blink {
namespace {

#ifndef NDEBUG
void AppendNodeToString(NGLayoutInputNode node,
                        StringBuilder* string_builder,
                        unsigned indent = 2) {
  if (!node)
    return;
  DCHECK(string_builder);

  string_builder->Append(node.ToString());
  string_builder->Append("\n");

  StringBuilder indent_builder;
  for (unsigned i = 0; i < indent; i++)
    indent_builder.Append(" ");

  if (node.IsBlock()) {
    NGLayoutInputNode first_child = ToNGBlockNode(node).FirstChild();
    for (NGLayoutInputNode node_runner = first_child; node_runner;
         node_runner = node_runner.NextSibling()) {
      string_builder->Append(indent_builder.ToString());
      AppendNodeToString(node_runner, string_builder, indent + 2);
    }
  }

  if (node.IsInline()) {
    for (const NGInlineItem& inline_item : ToNGInlineNode(node).Items()) {
      string_builder->Append(indent_builder.ToString());
      string_builder->Append(inline_item.ToString());
      string_builder->Append("\n");
    }
    NGLayoutInputNode next_sibling = ToNGInlineNode(node).NextSibling();
    for (NGLayoutInputNode node_runner = next_sibling; node_runner;
         node_runner = node_runner.NextSibling()) {
      string_builder->Append(indent_builder.ToString());
      AppendNodeToString(node_runner, string_builder, indent + 2);
    }
  }
}
#endif

}  // namespace

bool NGLayoutInputNode::IsInline() const {
  return type_ == kInline;
}

bool NGLayoutInputNode::IsBlock() const {
  return type_ == kBlock;
}

bool NGLayoutInputNode::IsFloating() const {
  return IsBlock() && Style().IsFloating();
}

bool NGLayoutInputNode::IsOutOfFlowPositioned() const {
  return IsBlock() && Style().HasOutOfFlowPosition();
}

bool NGLayoutInputNode::CreatesNewFormattingContext() const {
  return box_->AvoidsFloats();
}

bool NGLayoutInputNode::IsReplaced() const {
  return box_->IsLayoutReplaced();
}

RefPtr<NGLayoutResult> NGLayoutInputNode::Layout(NGConstraintSpace* space,
                                                 NGBreakToken* break_token) {
  return IsInline() ? ToNGInlineNode(*this).Layout(space, break_token)
                    : ToNGBlockNode(*this).Layout(space, break_token);
}

MinMaxContentSize NGLayoutInputNode::ComputeMinMaxContentSize() {
  return IsInline() ? ToNGInlineNode(*this).ComputeMinMaxContentSize()
                    : ToNGBlockNode(*this).ComputeMinMaxContentSize();
}

NGIntrinsicSize NGLayoutInputNode::IntrinsicSize() const {
  DCHECK(IsReplaced());

  // Get intrinsic sizing information from Legacy.
  NGIntrinsicSize intrinsic_size;

  LayoutSize default_intrinsic_size = box_->IntrinsicSize();
  // Transform to logical coordinates if needed.
  if (!Style().IsHorizontalWritingMode())
    default_intrinsic_size = default_intrinsic_size.TransposedSize();
  intrinsic_size.default_size = NGLogicalSize(default_intrinsic_size.Width(),
                                              default_intrinsic_size.Height());

  LayoutReplaced::IntrinsicSizingInfo legacy_sizing_info;

  ToLayoutReplaced(box_)->ComputeIntrinsicSizingInfo(legacy_sizing_info);
  if (legacy_sizing_info.has_width)
    intrinsic_size.computed_inline_size =
        LayoutUnit(legacy_sizing_info.size.Width());
  if (legacy_sizing_info.has_height)
    intrinsic_size.computed_block_size =
        LayoutUnit(legacy_sizing_info.size.Height());
  intrinsic_size.aspect_ratio =
      NGLogicalSize(LayoutUnit(legacy_sizing_info.aspect_ratio.Width()),
                    LayoutUnit(legacy_sizing_info.aspect_ratio.Height()));

  return intrinsic_size;
}

NGLayoutInputNode NGLayoutInputNode::NextSibling() {
  return IsInline() ? ToNGInlineNode(*this).NextSibling()
                    : ToNGBlockNode(*this).NextSibling();
}

LayoutObject* NGLayoutInputNode::GetLayoutObject() const {
  return box_;
}

const ComputedStyle& NGLayoutInputNode::Style() const {
  return box_->StyleRef();
}

String NGLayoutInputNode::ToString() const {
  return IsInline() ? ToNGInlineNode(*this).ToString()
                    : ToNGBlockNode(*this).ToString();
}

#ifndef NDEBUG
void NGLayoutInputNode::ShowNodeTree() const {
  StringBuilder string_builder;
  string_builder.Append("\n.:: LayoutNG Node Tree ::.\n\n");
  AppendNodeToString(*this, &string_builder);
  fprintf(stderr, "%s\n", string_builder.ToString().Utf8().data());
}
#endif

}  // namespace blink
