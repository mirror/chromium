// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/ng_constraint_space.h"

#include "core/layout/ng/ng_units.h"

namespace blink {

// TODO: This should set the size of the NGPhysicalConstraintSpace. Or we could
// remove it requiring that a NGConstraintSpace is created from a
// NGPhysicalConstraintSpace.
NGConstraintSpace::NGConstraintSpace(NGWritingMode writing_mode,
                                     NGLogicalSize container_size)
    : physical_space_(new NGPhysicalConstraintSpace(
          container_size.ConvertToPhysical(writing_mode))),
      size_(container_size),
      writing_mode_(writing_mode) {}

NGConstraintSpace::NGConstraintSpace(NGWritingMode writing_mode,
                                     NGPhysicalConstraintSpace* physical_space)
    : physical_space_(physical_space),
      size_(physical_space->ContainerSize().ConvertToLogical(writing_mode)),
      writing_mode_(writing_mode) {}

NGConstraintSpace::NGConstraintSpace(NGWritingMode writing_mode,
                                     const NGConstraintSpace* constraint_space)
    : physical_space_(constraint_space->PhysicalSpace()),
      offset_(constraint_space->Offset()),
      size_(constraint_space->Size()),
      writing_mode_(writing_mode) {}

NGConstraintSpace::NGConstraintSpace(NGWritingMode writing_mode,
                                     const NGConstraintSpace& other,
                                     NGLogicalOffset offset,
                                     NGLogicalSize size)
    : physical_space_(other.PhysicalSpace()),
      offset_(offset),
      size_(size),
      writing_mode_(writing_mode) {}

NGConstraintSpace::NGConstraintSpace(const NGConstraintSpace& other,
                                     NGLogicalOffset offset,
                                     NGLogicalSize size)
    : physical_space_(other.PhysicalSpace()),
      offset_(offset),
      size_(size),
      writing_mode_(other.WritingMode()) {}

NGConstraintSpace* NGConstraintSpace::CreateFromLayoutObject(
    const LayoutBox& box) {
  bool fixed_inline = false, fixed_block = false;
  // XXX for orthogonal writing mode this is not right
  LayoutUnit container_logical_width =
      std::max(LayoutUnit(), box.containingBlockLogicalWidthForContent());
  // XXX Make sure this height is correct
  LayoutUnit container_logical_height =
      box.containingBlockLogicalHeightForContent(ExcludeMarginBorderPadding);
  if (box.hasOverrideLogicalContentWidth()) {
    container_logical_width = box.overrideLogicalContentWidth();
    fixed_inline = true;
  }
  if (box.hasOverrideLogicalContentHeight()) {
    container_logical_width = box.overrideLogicalContentHeight();
    fixed_block = true;
  }

  NGConstraintSpace* derived_constraint_space = new NGConstraintSpace(
      FromPlatformWritingMode(box.styleRef().getWritingMode()),
      NGLogicalSize(container_logical_width, container_logical_height));
  derived_constraint_space->SetOverflowTriggersScrollbar(
      box.styleRef().overflowInlineDirection() == OverflowAuto,
      box.styleRef().overflowBlockDirection() == OverflowAuto);
  derived_constraint_space->SetFixedSize(fixed_inline, fixed_block);
  return derived_constraint_space;
}

NGLogicalSize NGConstraintSpace::ContainerSize() const {
  return physical_space_->container_size_.ConvertToLogical(
      static_cast<NGWritingMode>(writing_mode_));
}

bool NGConstraintSpace::InlineTriggersScrollbar() const {
  return writing_mode_ == HorizontalTopBottom
             ? physical_space_->width_direction_triggers_scrollbar_
             : physical_space_->height_direction_triggers_scrollbar_;
}

bool NGConstraintSpace::BlockTriggersScrollbar() const {
  return writing_mode_ == HorizontalTopBottom
             ? physical_space_->height_direction_triggers_scrollbar_
             : physical_space_->width_direction_triggers_scrollbar_;
}

bool NGConstraintSpace::FixedInlineSize() const {
  return writing_mode_ == HorizontalTopBottom ? physical_space_->fixed_width_
                                              : physical_space_->fixed_height_;
}

bool NGConstraintSpace::FixedBlockSize() const {
  return writing_mode_ == HorizontalTopBottom ? physical_space_->fixed_height_
                                              : physical_space_->fixed_width_;
}

NGFragmentationType NGConstraintSpace::BlockFragmentationType() const {
  return static_cast<NGFragmentationType>(
      writing_mode_ == HorizontalTopBottom
          ? physical_space_->height_direction_fragmentation_type_
          : physical_space_->width_direction_fragmentation_type_);
}

void NGConstraintSpace::Subtract(const NGFragment*) {
  // TODO(layout-ng): Implement.
}

NGLayoutOpportunityIterator NGConstraintSpace::LayoutOpportunities(
    unsigned clear,
    bool for_inline_or_bfc) {
  NGLayoutOpportunityIterator iterator(this, clear, for_inline_or_bfc);
  return iterator;
}

void NGConstraintSpace::SetOverflowTriggersScrollbar(bool inline_triggers,
                                                     bool block_triggers) {
  if (writing_mode_ == HorizontalTopBottom) {
    physical_space_->width_direction_triggers_scrollbar_ = inline_triggers;
    physical_space_->height_direction_triggers_scrollbar_ = block_triggers;
  } else {
    physical_space_->width_direction_triggers_scrollbar_ = block_triggers;
    physical_space_->height_direction_triggers_scrollbar_ = inline_triggers;
  }
}

void NGConstraintSpace::SetFixedSize(bool inline_fixed, bool block_fixed) {
  if (writing_mode_ == HorizontalTopBottom) {
    physical_space_->fixed_width_ = inline_fixed;
    physical_space_->fixed_height_ = block_fixed;
  } else {
    physical_space_->fixed_width_ = block_fixed;
    physical_space_->fixed_height_ = inline_fixed;
  }
}

void NGConstraintSpace::SetFragmentationType(NGFragmentationType type) {
  if (writing_mode_ == HorizontalTopBottom) {
    DCHECK_EQ(static_cast<NGFragmentationType>(
                  physical_space_->width_direction_fragmentation_type_),
              FragmentNone);
    physical_space_->height_direction_fragmentation_type_ = type;
  } else {
    DCHECK_EQ(static_cast<NGFragmentationType>(
                  physical_space_->height_direction_fragmentation_type_),
              FragmentNone);
    physical_space_->width_direction_triggers_scrollbar_ = type;
  }
}

NGConstraintSpace* NGLayoutOpportunityIterator::Next() {
  auto* exclusions = constraint_space_->PhysicalSpace()->Exclusions();
  if (!exclusions->head())
    return new NGConstraintSpace(constraint_space_->WritingMode(),
                                 constraint_space_->PhysicalSpace());
  return nullptr;
}

}  // namespace blink
