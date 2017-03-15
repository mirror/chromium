// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/ng_inline_node.h"

#include "core/layout/LayoutBlockFlow.h"
#include "core/layout/LayoutObject.h"
#include "core/layout/LayoutText.h"
#include "core/layout/ng/ng_bidi_paragraph.h"
#include "core/layout/ng/ng_box_fragment.h"
#include "core/layout/ng/ng_constraint_space_builder.h"
#include "core/layout/ng/ng_fragment_builder.h"
#include "core/layout/ng/ng_layout_inline_items_builder.h"
#include "core/layout/ng/ng_line_builder.h"
#include "core/layout/ng/ng_physical_box_fragment.h"
#include "core/layout/ng/ng_physical_text_fragment.h"
#include "core/layout/ng/ng_text_fragment.h"
#include "core/layout/ng/ng_text_layout_algorithm.h"
#include "core/style/ComputedStyle.h"
#include "platform/fonts/CharacterRange.h"
#include "platform/fonts/shaping/CachingWordShapeIterator.h"
#include "platform/fonts/shaping/CachingWordShaper.h"
#include "platform/fonts/shaping/HarfBuzzShaper.h"
#include "platform/fonts/shaping/ShapeResultBuffer.h"
#include "wtf/text/CharacterNames.h"

namespace blink {

NGInlineNode::NGInlineNode(LayoutObject* start_inline, LayoutBlockFlow* block)
    : NGLayoutInputNode(NGLayoutInputNodeType::kLegacyInline),
      start_inline_(start_inline),
      block_(block) {
  DCHECK(start_inline);
  DCHECK(block);
}

NGInlineNode::NGInlineNode()
    : NGLayoutInputNode(NGLayoutInputNodeType::kLegacyInline),
      start_inline_(nullptr),
      block_(nullptr) {}

NGInlineNode::~NGInlineNode() {}

NGLayoutInlineItemRange NGInlineNode::Items(unsigned start, unsigned end) {
  return NGLayoutInlineItemRange(&items_, start, end);
}

void NGInlineNode::InvalidatePrepareLayout() {
  text_content_ = String();
  items_.clear();
}

void NGInlineNode::PrepareLayout() {
  // Scan list of siblings collecting all in-flow non-atomic inlines. A single
  // NGInlineNode represent a collection of adjacent non-atomic inlines.
  CollectInlines(start_inline_, block_);
  if (is_bidi_enabled_)
    SegmentText();
  ShapeText();
}

// Depth-first-scan of all LayoutInline and LayoutText nodes that make up this
// NGInlineNode object. Collects LayoutText items, merging them up into the
// parent LayoutInline where possible, and joining all text content in a single
// string to allow bidi resolution and shaping of the entire block.
void NGInlineNode::CollectInlines(LayoutObject* start, LayoutBlockFlow* block) {
  DCHECK(text_content_.isNull());
  DCHECK(items_.isEmpty());
  NGLayoutInlineItemsBuilder builder(&items_);
  builder.EnterBlock(block->style());
  LayoutObject* next_sibling = CollectInlines(start, block, &builder);
  builder.ExitBlock();

  text_content_ = builder.ToString();
  DCHECK(!next_sibling || !next_sibling->isInline());
  next_sibling_ = next_sibling ? new NGBlockNode(next_sibling) : nullptr;
  is_bidi_enabled_ = !text_content_.isEmpty() &&
                     !(text_content_.is8Bit() && !builder.HasBidiControls());
}

LayoutObject* NGInlineNode::CollectInlines(
    LayoutObject* start,
    LayoutBlockFlow* block,
    NGLayoutInlineItemsBuilder* builder) {
  LayoutObject* node = start;
  while (node) {
    if (node->isText()) {
      builder->SetIsSVGText(node->isSVGInlineText());
      builder->Append(toLayoutText(node)->text(), node->style(), node);
      node->clearNeedsLayout();

    } else if (node->isFloating() || node->isOutOfFlowPositioned()) {
      // Add floats and positioned objects in the same way as atomic inlines.
      // Because these objects need positions, they will be handled in
      // NGLineBuilder.
      builder->Append(objectReplacementCharacter, nullptr, node);

    } else if (!node->isInline()) {
      // A block box found. End inline and transit to block layout.
      return node;

    } else {
      builder->EnterInline(node);

      // For atomic inlines add a unicode "object replacement character" to
      // signal the presence of a non-text object to the unicode bidi algorithm.
      if (node->isAtomicInlineLevel()) {
        builder->Append(objectReplacementCharacter, nullptr, node);
      }

      // Otherwise traverse to children if they exist.
      else if (LayoutObject* child = node->slowFirstChild()) {
        node = child;
        continue;

      } else {
        // An empty inline node.
        node->clearNeedsLayout();
      }

      builder->ExitInline(node);
    }

    // Find the next sibling, or parent, until we reach |block|.
    while (true) {
      if (LayoutObject* next = node->nextSibling()) {
        node = next;
        break;
      }
      node = node->parent();
      builder->ExitInline(node);
      if (node == block)
        return nullptr;
      DCHECK(node->isInline());
      node->clearNeedsLayout();
    }
  }
  return nullptr;
}

void NGInlineNode::SegmentText() {
  // TODO(kojii): Move this to caller, this will be used again after line break.
  NGBidiParagraph bidi;
  text_content_.ensure16Bit();
  if (!bidi.SetParagraph(text_content_, BlockStyle())) {
    // On failure, give up bidi resolving and reordering.
    is_bidi_enabled_ = false;
    return;
  }
  if (bidi.Direction() == UBIDI_LTR) {
    // All runs are LTR, no need to reorder.
    is_bidi_enabled_ = false;
    return;
  }

  unsigned item_index = 0;
  for (unsigned start = 0; start < text_content_.length();) {
    UBiDiLevel level;
    unsigned end = bidi.GetLogicalRun(start, &level);
    DCHECK_EQ(items_[item_index].start_offset_, start);
    item_index =
        NGLayoutInlineItem::SetBidiLevel(items_, item_index, end, level);
    start = end;
  }
  DCHECK_EQ(item_index, items_.size());
}

// Set bidi level to a list of NGLayoutInlineItem from |index| to the item that
// ends with |end_offset|.
// If |end_offset| is mid of an item, the item is split to ensure each item has
// one bidi level.
// @param items The list of NGLayoutInlineItem.
// @param index The first index of the list to set.
// @param end_offset The exclusive end offset to set.
// @param level The level to set.
// @return The index of the next item.
unsigned NGLayoutInlineItem::SetBidiLevel(Vector<NGLayoutInlineItem>& items,
                                          unsigned index,
                                          unsigned end_offset,
                                          UBiDiLevel level) {
  for (; items[index].end_offset_ < end_offset; index++)
    items[index].bidi_level_ = level;
  items[index].bidi_level_ = level;
  if (items[index].end_offset_ > end_offset)
    Split(items, index, end_offset);
  return index + 1;
}

// Split |items[index]| to 2 items at |offset|.
// All properties other than offsets are copied to the new item and it is
// inserted at |items[index + 1]|.
// @param items The list of NGLayoutInlineItem.
// @param index The index to split.
// @param offset The offset to split at.
void NGLayoutInlineItem::Split(Vector<NGLayoutInlineItem>& items,
                               unsigned index,
                               unsigned offset) {
  DCHECK_GT(offset, items[index].start_offset_);
  DCHECK_LT(offset, items[index].end_offset_);
  items.insert(index + 1, items[index]);
  items[index].end_offset_ = offset;
  items[index + 1].start_offset_ = offset;
}

void NGLayoutInlineItem::SetEndOffset(unsigned end_offset) {
  DCHECK_GE(end_offset, start_offset_);
  end_offset_ = end_offset;
}

LayoutUnit NGLayoutInlineItem::InlineSize() const {
  return InlineSize(start_offset_, end_offset_);
}

LayoutUnit NGLayoutInlineItem::InlineSize(unsigned start, unsigned end) const {
  DCHECK(start >= StartOffset() && start <= end && end <= EndOffset());

  if (start == end)
    return LayoutUnit();

  if (!style_ || !shape_result_) {
    // Bidi controls do not have widths.
    // TODO(kojii): Atomic inline not supported yet.
    return LayoutUnit();
  }

  if (start == start_offset_ && end == end_offset_)
    return LayoutUnit(shape_result_->width());

  return LayoutUnit(ShapeResultBuffer::getCharacterRange(
                        shape_result_, Direction(), shape_result_->width(),
                        start - StartOffset(), end - StartOffset())
                        .width());
}

void NGLayoutInlineItem::GetFallbackFonts(
    HashSet<const SimpleFontData*>* fallback_fonts,
    unsigned start,
    unsigned end) const {
  DCHECK(start >= StartOffset() && start <= end && end <= EndOffset());

  // TODO(kojii): Implement |start| and |end|.
  shape_result_->fallbackFonts(fallback_fonts);
}

void NGInlineNode::ShapeText() {
  // TODO(eae): Add support for shaping latin-1 text?
  text_content_.ensure16Bit();

  // Shape each item with the full context of the entire node.
  HarfBuzzShaper shaper(text_content_.characters16(), text_content_.length());
  for (auto& item : items_) {
    // Skip object replacement characters and bidi control characters.
    if (!item.style_)
      continue;

    item.shape_result_ = shaper.shape(&item.Style()->font(), item.Direction(),
                                      item.StartOffset(), item.EndOffset());
  }
}

RefPtr<NGLayoutResult> NGInlineNode::Layout(NGConstraintSpace*, NGBreakToken*) {
  ASSERT_NOT_REACHED();
  return nullptr;
}

void NGInlineNode::LayoutInline(NGConstraintSpace* constraint_space,
                                NGLineBuilder* line_builder) {
  if (!IsPrepareLayoutFinished())
    PrepareLayout();

  if (text_content_.isEmpty())
    return;

  NGTextLayoutAlgorithm(this, constraint_space).LayoutInline(line_builder);
}

MinMaxContentSize NGInlineNode::ComputeMinMaxContentSize() {
  // Compute the max of inline sizes of all line boxes with 0 available inline
  // size. This gives the min-content, the width where lines wrap at every break
  // opportunity.
  NGWritingMode writing_mode =
      FromPlatformWritingMode(BlockStyle()->getWritingMode());
  RefPtr<NGConstraintSpace> constraint_space =
      NGConstraintSpaceBuilder(writing_mode)
          .SetTextDirection(BlockStyle()->direction())
          .SetAvailableSize({LayoutUnit(), NGSizeIndefinite})
          .ToConstraintSpace(writing_mode);
  NGLineBuilder line_builder(this, constraint_space.get(), nullptr);
  LayoutInline(constraint_space.get(), &line_builder);
  MinMaxContentSize sizes;
  sizes.min_content = line_builder.MaxInlineSize();

  // max-content is the width without any line wrapping.
  // TODO(kojii): Implement hard breaks (<br> etc.) to break.
  for (const auto& item : items_)
    sizes.max_content += item.InlineSize();

  return sizes;
}

NGLayoutInputNode* NGInlineNode::NextSibling() {
  if (!IsPrepareLayoutFinished())
    PrepareLayout();
  return next_sibling_;
}

LayoutObject* NGInlineNode::GetLayoutObject() {
  return GetLayoutBlockFlow();
}

// Compute the delta of text offsets between NGInlineNode and LayoutText.
// This map is needed to produce InlineTextBox since its offsets are to
// LayoutText.
// TODO(kojii): Since NGInlineNode has text after whitespace collapsed, the
// length may not match with LayoutText. This function updates LayoutText to
// match, but this needs more careful coding, if we keep copying to layoutobject
// tree.
void NGInlineNode::GetLayoutTextOffsets(
    Vector<unsigned, 32>* text_offsets_out) {
  LayoutText* current_text = nullptr;
  unsigned current_offset = 0;
  for (unsigned i = 0; i < items_.size(); i++) {
    const NGLayoutInlineItem& item = items_[i];
    LayoutObject* next_object = item.GetLayoutObject();
    LayoutText* next_text = next_object && next_object->isText()
                                ? toLayoutText(next_object)
                                : nullptr;
    if (next_text != current_text) {
      if (current_text &&
          current_text->textLength() != item.StartOffset() - current_offset) {
        current_text->setTextInternal(
            Text(current_offset, item.StartOffset()).impl());
      }
      current_text = next_text;
      current_offset = item.StartOffset();
    }
    (*text_offsets_out)[i] = current_offset;
  }
  if (current_text &&
      current_text->textLength() != text_content_.length() - current_offset) {
    current_text->setTextInternal(
        Text(current_offset, text_content_.length()).impl());
  }
}

DEFINE_TRACE(NGInlineNode) {
  visitor->trace(next_sibling_);
  NGLayoutInputNode::trace(visitor);
}

NGLayoutInlineItemRange::NGLayoutInlineItemRange(
    Vector<NGLayoutInlineItem>* items,
    unsigned start_index,
    unsigned end_index)
    : start_item_(&(*items)[start_index]),
      size_(end_index - start_index),
      start_index_(start_index) {
  RELEASE_ASSERT(start_index <= end_index && end_index <= items->size());
}

}  // namespace blink
