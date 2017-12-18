// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/floats/ng_exclusion_space.h"

#include "core/layout/ng/ng_exclusion.h"

namespace blink {

namespace {

bool HasSolidEdges(const Vector<scoped_refptr<const NGExclusion>, 1>& edges,
                   LayoutUnit block_start,
                   LayoutUnit block_end) {
  // If there aren't any adjacent exclusions, we must be the initial shelf.
  // This always has "solid" edges on either side.
  if (edges.IsEmpty())
    return true;

  for (const auto& edge : edges) {
    if (edge->rect.BlockEndOffset() > block_start &&
        edge->rect.BlockStartOffset() < block_end)
      return true;
  }

  return false;
}

void CollectSolidEdges(const Vector<scoped_refptr<const NGExclusion>, 1>& edges,
                       LayoutUnit block_offset,
                       Vector<scoped_refptr<const NGExclusion>, 1>* new_edges) {
  for (const auto& edge : edges) {
    if (edge->rect.BlockEndOffset() > block_offset)
      new_edges->push_back(edge);
  }
}

bool Intersects(const NGLayoutOpportunity& opportunity,
                const NGBfcOffset& offset,
                const LayoutUnit inline_size) {
  return opportunity.rect.LineEndOffset() > offset.line_offset &&
         opportunity.rect.LineStartOffset() <
             offset.line_offset + inline_size &&
         opportunity.rect.BlockEndOffset() > offset.block_offset;
}

bool Intersects(const NGExclusionSpace::NGShelf& shelf,
                const NGBfcOffset& offset,
                const LayoutUnit inline_size) {
  return shelf.line_right > offset.line_offset &&
         shelf.line_left < offset.line_offset + inline_size;
}

NGLayoutOpportunity CreateLayoutOpportunity(const NGLayoutOpportunity& other,
                                            const NGBfcOffset& offset,
                                            const LayoutUnit inline_size) {
  NGLayoutOpportunity opportunity;
  opportunity.rect.start_offset.line_offset =
      std::max(other.rect.LineStartOffset(), offset.line_offset);
  opportunity.rect.start_offset.block_offset =
      std::max(other.rect.start_offset.block_offset, offset.block_offset);

  opportunity.rect.end_offset.line_offset =
      std::min(other.rect.LineEndOffset(), offset.line_offset + inline_size);
  opportunity.rect.end_offset.block_offset = other.rect.BlockEndOffset();

  return opportunity;
}

NGLayoutOpportunity CreateLayoutOpportunity(
    const NGExclusionSpace::NGShelf& shelf,
    const NGBfcOffset& offset,
    const LayoutUnit inline_size) {
  NGLayoutOpportunity opportunity;
  opportunity.rect.start_offset.line_offset =
      std::max(shelf.line_left, offset.line_offset);
  opportunity.rect.start_offset.block_offset =
      std::max(shelf.block_offset, offset.block_offset);

  opportunity.rect.end_offset.line_offset =
      std::min(shelf.line_right, offset.line_offset + inline_size);
  opportunity.rect.end_offset.block_offset = LayoutUnit::Max();

  return opportunity;
}

}  // namespace

NGExclusionSpace::NGExclusionSpace()
    : last_float_block_start_(LayoutUnit::Min()),
      left_float_clear_offset_(LayoutUnit::Min()),
      right_float_clear_offset_(LayoutUnit::Min()),
      has_left_float_(false),
      has_right_float_(false) {
  NGShelf shelf;
  shelf.block_offset = LayoutUnit::Min();
  shelf.line_left = LayoutUnit::Min();
  shelf.line_right = LayoutUnit::Max();

  shelves_.push_back(shelf);
}

void NGExclusionSpace::Add(scoped_refptr<const NGExclusion> exclusion) {
  last_float_block_start_ =
      std::max(last_float_block_start_, exclusion->rect.BlockStartOffset());

  // Update the members used for clearance calculations.
  if (exclusion->type == EFloat::kLeft) {
    has_left_float_ = true;
    left_float_clear_offset_ =
        std::max(left_float_clear_offset_, exclusion->rect.BlockEndOffset());
  } else if (exclusion->type == EFloat::kRight) {
    has_right_float_ = true;
    right_float_clear_offset_ =
        std::max(right_float_clear_offset_, exclusion->rect.BlockEndOffset());
  }

  // Modify the shelves and opportunities given this new opportunity.
  bool inserted = false;
  for (size_t i = 0; i < shelves_.size(); ++i) {
    NGShelf& shelf = shelves_[i];

    // We need to keep a copy of the shelf if we need to insert a new shelf
    // later in the loop.
    NGShelf shelf_copy(shelf);

    // Check if the new exclusion will be below this shelf. E.g.
    //
    //    0 1 2 3 4 5 6 7 8
    // 0  +---+
    //    |xxx|
    // 10 |xxx|
    //    X---------------X
    // 20          +---+
    //             |NEW|
    //             +---+
    bool is_below = exclusion->rect.BlockStartOffset() > shelf.block_offset;

    if (is_below) {
      // We may have created a new opportunity, by closing off an area.
      // However we need to check the "edges" of the opportunity are solid.
      //
      //    0 1 2 3 4 5 6 7 8
      // 0  +---+  X----X+---+
      //    |xxx|  .     |xxx|
      // 10 |xxx|  .     |xxx|
      //    +---+  .     +---+
      // 20        .     .
      //      +---+. .+---+
      // 30   |xxx|   |NEW|
      //      |xxx|   +---+
      // 40   +---+
      //
      // In the above example we have three exclusions placed in the space. And
      // we are adding the "NEW" right exclusion.
      //
      // The drawn "shelf" has the internal values:
      // {
      //   block_offset: 0,
      //   line_left: 35,
      //   line_right: 60,
      //   line_left_edges: [{25, 40}],
      //   line_right_edges: [{0, 15}],
      // }
      // The hypothetical opportunity starts at (35,0), and ends at (60,25).
      //
      // The new exclusion *doesn't* create a new layout opportunity, as the
      // left edge doesn't have a solid "edge", i.e. there are no floats
      // against that edge.
      bool has_solid_edges =
          HasSolidEdges(shelf.line_left_edges, shelf.block_offset,
                        exclusion->rect.BlockStartOffset()) &&
          HasSolidEdges(shelf.line_right_edges, shelf.block_offset,
                        exclusion->rect.BlockStartOffset());

      // This just checks if the exclusion overlaps the bounds of the shelf.
      //
      //    0 1 2 3 4 5 6 7 8
      // 0  +---+X------X+---+
      //    |xxx|        |xxx|
      // 10 |xxx|        |xxx|
      //    +---+        +---+
      // 20
      //                  +---+
      // 30               |NEW|
      //                  +---+
      //
      // In the above example the "NEW" exclusion *doesn't* overlap with the
      // above drawn shelf.
      bool is_overlapping =
          exclusion->rect.LineStartOffset() < shelf.line_right &&
          exclusion->rect.LineEndOffset() > shelf.line_left;

      if (has_solid_edges && is_overlapping) {
        // TODO insert in the correct location.
        bool inserted_opportunity = false;
        NGLayoutOpportunity opportunity;
        opportunity.rect =
            NGBfcRect({shelf.line_left, shelf.block_offset},
                      {shelf.line_right, exclusion->rect.BlockStartOffset()});

        // We go backwards through the oppo...TODO comment.
        if (opportunities_.size()) {
          for (size_t j = opportunities_.size() - 1; j >= 0; --j) {
            if (opportunities_[j].rect.BlockStartOffset() <=
                opportunity.rect.BlockStartOffset()) {
              opportunities_.insert(j + 1, opportunity);
              inserted_opportunity = true;
              break;
            }
          }
        }

        // HMMM THINK TODO
        if (!inserted_opportunity)
          opportunities_.push_back(opportunity);
      }
    }

    // Check if the new exclusion is going to "cut" (intersect) with this
    // shelf. E.g.
    //
    //    0 1 2 3 4 5 6 7 8
    // 0  +---+
    //    |xxx|
    // 10 |xxx|    +---+
    //    X--------|NEW|--X
    //             +---+
    bool is_intersecting =
        exclusion->rect.BlockStartOffset() <= shelf.block_offset &&
        exclusion->rect.BlockEndOffset() > shelf.block_offset;

    // We need to reduce the size of the shelf.
    if (is_below || is_intersecting) {
      if (exclusion->type == EFloat::kLeft) {
        // The edges need to be cleared if it pushes the shelf edge in.
        if (exclusion->rect.LineEndOffset() > shelf.line_left)
          shelf.line_left_edges.clear();

        if (exclusion->rect.LineEndOffset() >= shelf.line_left) {
          shelf.line_left = exclusion->rect.LineEndOffset();
          shelf.line_left_edges.push_back(exclusion);
        }
      } else {
        DCHECK_EQ(exclusion->type, EFloat::kRight);
        if (exclusion->rect.LineStartOffset() < shelf.line_right)
          shelf.line_right_edges.clear();

        if (exclusion->rect.LineStartOffset() <= shelf.line_right) {
          shelf.line_right = exclusion->rect.LineStartOffset();
          shelf.line_right_edges.push_back(exclusion);
        }
      }

      // We can end up in a situation where a shelf is the same as the previous
      // one. For example:
      //
      //    0 1 2 3 4 5 6 7 8
      // 0  +---+X----------X
      //    |xxx|
      // 10 |xxx|
      //    X---------------X
      // 20
      //      +---+
      // 30   |NEW|
      //      +---+
      //
      // In the above example "NEW" will shrink the two shelves by the same
      // amount. We remove the current shelf we are working on.
      bool is_same_as_previous = (i > 0) &&
                                 shelf.line_left == shelves_[i - 1].line_left &&
                                 shelf.line_right == shelves_[i - 1].line_right;
      // TODO differs from the prototype version, need to add DCHECKS.

      // The shelf also may now be non-existent.
      bool shelf_has_zero_size = shelf.line_right <= shelf.line_left;

      if (is_same_as_previous || shelf_has_zero_size) {
        shelves_.EraseAt(i--);
        continue;
      }
    }

    const LayoutUnit exclusion_end_offset = exclusion->rect.BlockEndOffset();

    // We only want to insert a new shelf if it's going to be placed in between
    // two other shelves. E.g.
    //
    //    0 1 2 3 4 5 6 7 8
    // 0  +-----+X----X+---+
    //    |xxxxx|      |xxx|
    // 10 +-----+      |xxx|
    //      +---+      |xxx|
    // 20   |NEW|      |xxx|
    //    X-----------X|xxx|
    // 30              |xxx|
    //    X----------------X
    //
    // In the above example the "NEW" left exclusion creates a shelf between
    // the two other shelves drawn.
    bool is_between_shelves =
        exclusion_end_offset >= shelf.block_offset && i + 1 < shelves_.size() &&
        exclusion_end_offset < shelves_[i + 1].block_offset;

    if (is_between_shelves) {
      DCHECK(!inserted);
      inserted = true;

      // We don't want to add the shelf if its at exactly the same block offset.
      if (exclusion_end_offset != shelf.block_offset) {
        NGShelf new_shelf;
        new_shelf.block_offset = exclusion->rect.BlockEndOffset();

        // TODO(ikilpatrick): There may be cases where we need to do this in a
        // few certain circumstances.
        CollectSolidEdges(shelf_copy.line_left_edges, new_shelf.block_offset,
                          &new_shelf.line_left_edges);

        CollectSolidEdges(shelf_copy.line_right_edges, new_shelf.block_offset,
                          &new_shelf.line_right_edges);

        // If we didn't find any edges, the line_left/line_right of the shelf
        // are pushed out to be the minimum/maximum.
        new_shelf.line_left = new_shelf.line_left_edges.IsEmpty()
                                  ? LayoutUnit::Min()
                                  : shelf_copy.line_left;
        new_shelf.line_right = new_shelf.line_right_edges.IsEmpty()
                                   ? LayoutUnit::Max()
                                   : shelf_copy.line_right;

        shelves_.insert(i + 1, new_shelf);
      }

      // Its safe to early exit out of this loop now. This exclusion won't
      // have any affect on subsequent shelves.
      break;
    }
  }

  // We've gone through all the shelves and didn't insert a new shelf
  // in-between any of them. So we now insert one at the very end.
  if (!inserted) {
    NGShelf new_shelf;
    new_shelf.line_left = LayoutUnit::Min();
    new_shelf.line_right = LayoutUnit::Max();
    new_shelf.block_offset = exclusion->rect.BlockEndOffset();

    shelves_.push_back(new_shelf);
  }

  exclusions_.push_back(std::move(exclusion));
}

NGLayoutOpportunity NGExclusionSpace::FindLayoutOpportunity(
    const NGBfcOffset& offset,
    const NGLogicalSize& available_size,
    const NGLogicalSize& minimum_size) const {
  Vector<NGLayoutOpportunity> opportunities =
      AllLayoutOpportunities(offset, available_size);

  for (auto opportunity : opportunities) {
    // TODO comment.
    if ((opportunity.rect.InlineSize() >= minimum_size.inline_size ||
         opportunity.rect.InlineSize() == available_size.inline_size) &&
        opportunity.rect.BlockSize() >= minimum_size.block_size) {
      return opportunity;
    }
  }

  // TODO comment.
  if (offset.line_offset == LayoutUnit::Max()) {
    return CreateLayoutOpportunity(*shelves_.end(), offset, LayoutUnit());
  }

  NOTREACHED();
  return NGLayoutOpportunity();
}

Vector<NGLayoutOpportunity> NGExclusionSpace::AllLayoutOpportunities(
    const NGBfcOffset& offset,
    const NGLogicalSize& available_size) const {
  const LayoutUnit inline_size = available_size.inline_size;
  Vector<NGLayoutOpportunity> opportunities;

  auto shelves_it = shelves_.begin();
  auto opps_it = opportunities_.begin();

  const auto shelves_end = shelves_.end();
  const auto opps_end = opportunities_.end();

  while (shelves_it != shelves_.end() || opps_it != opportunities_.end()) {
    if (shelves_it != shelves_end && opps_it != opps_end) {
      const NGLayoutOpportunity& opportunity = *opps_it;
      const NGShelf& shelf = *shelves_it;

      if (!Intersects(opportunity, offset, inline_size)) {
        ++opps_it;
        continue;
      }

      if (!Intersects(shelf, offset, inline_size)) {
        ++shelves_it;
        continue;
      }

      if (opportunity.rect.BlockStartOffset() <= shelf.block_offset) {
        opportunities.push_back(
            CreateLayoutOpportunity(opportunity, offset, inline_size));
        ++opps_it;
      } else {
        bool has_solid_edges =
            HasSolidEdges(shelf.line_left_edges, offset.block_offset,
                          LayoutUnit::Max()) &&
            HasSolidEdges(shelf.line_right_edges, offset.block_offset,
                          LayoutUnit::Max());
        if (has_solid_edges) {
          opportunities.push_back(
              CreateLayoutOpportunity(shelf, offset, inline_size));
        }
        ++shelves_it;
      }
    } else if (shelves_it != shelves_end) {
      const NGShelf& shelf = *shelves_it;

      if (!Intersects(shelf, offset, inline_size)) {
        ++shelves_it;
        continue;
      }

      bool has_solid_edges =
          HasSolidEdges(shelf.line_left_edges, offset.block_offset,
                        LayoutUnit::Max()) &&
          HasSolidEdges(shelf.line_right_edges, offset.block_offset,
                        LayoutUnit::Max());
      if (has_solid_edges) {
        opportunities.push_back(
            CreateLayoutOpportunity(shelf, offset, inline_size));
      }
      ++shelves_it;
    } else {
      // We should never exhaust the opportunities list before the shelves list.
      NOTREACHED();
    }
  }

  return opportunities;
}

LayoutUnit NGExclusionSpace::ClearanceOffset(EClear clear_type) const {
  switch (clear_type) {
    case EClear::kNone:
      return LayoutUnit::Min();  // nothing to do here.
    case EClear::kLeft:
      return left_float_clear_offset_;
    case EClear::kRight:
      return right_float_clear_offset_;
    case EClear::kBoth:
      return std::max(left_float_clear_offset_, right_float_clear_offset_);
    default:
      NOTREACHED();
  }

  return LayoutUnit::Min();
}

bool NGExclusionSpace::operator==(const NGExclusionSpace& other) const {
  return exclusions_ == other.exclusions_;
}

}  // namespace blink
