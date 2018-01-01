// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGLineBreaker_h
#define NGLineBreaker_h

#include "core/CoreExport.h"
#include "core/layout/ng/inline/ng_inline_item_result.h"
#include "core/layout/ng/inline/ng_inline_node.h"
#include "core/layout/ng/ng_layout_opportunity_iterator.h"
#include "platform/fonts/shaping/HarfBuzzShaper.h"
#include "platform/fonts/shaping/ShapeResultSpacing.h"
#include "platform/text/TextBreakIterator.h"
#include "platform/wtf/Allocator.h"

namespace blink {

class Hyphenation;
class NGInlineBreakToken;
class NGInlineItem;
class NGInlineLayoutStateStack;
struct NGPositionedFloat;

// The line breaker needs to know which mode its in to properly handle floats.
enum class NGLineBreakerMode { kContent, kMinContent, kMaxContent };

// Represents a line breaker.
//
// This class measures each NGInlineItem and determines items to form a line,
// so that NGInlineLayoutAlgorithm can build a line box from the output.
class CORE_EXPORT NGLineBreaker {
  STACK_ALLOCATED();

 public:
  NGLineBreaker(NGInlineNode,
                NGLineBreakerMode,
                const NGConstraintSpace&,
                Vector<NGPositionedFloat>*,
                Vector<scoped_refptr<NGUnpositionedFloat>>*,
                NGExclusionSpace*,
                unsigned handled_float_index,
                const NGInlineBreakToken* = nullptr);
  ~NGLineBreaker() {}

  // Compute the next line break point and produces NGInlineItemResults for
  // the line.
  bool NextLine(const NGLayoutOpportunity&, NGLineInfo*);

  // Create an NGInlineBreakToken for the last line returned by NextLine().
  scoped_refptr<NGInlineBreakToken> CreateBreakToken(
      std::unique_ptr<const NGInlineLayoutStateStack>) const;

 private:
  // This struct holds information for the current line.
  struct LineData {
    STACK_ALLOCATED();

    // The current position from inline_start. Unlike NGInlineLayoutAlgorithm
    // that computes position in visual order, this position in logical order.
    LayoutUnit position;

    // The current opportunity.
    NGLayoutOpportunity opportunity;

    LayoutUnit line_left_bfc_offset;
    LayoutUnit line_right_bfc_offset;

    // We don't create "certain zero-height line boxes".
    // https://drafts.csswg.org/css2/visuren.html#phantom-line-box
    // Such line boxes do not prevent two margins being "adjoining", and thus
    // collapsing.
    // https://drafts.csswg.org/css2/box.html#collapsing-margins
    bool should_create_line_box = false;

    // Set when the line ended with a forced break. Used to setup the states for
    // the next line.
    bool has_forced_break = false;

    LayoutUnit AvailableWidth() const {
      DCHECK_GE(line_right_bfc_offset, line_left_bfc_offset);
      return line_right_bfc_offset - line_left_bfc_offset;
    }
    bool CanFit() const { return position <= AvailableWidth(); }
    bool CanFit(LayoutUnit extra) const {
      return position + extra <= AvailableWidth();
    }
  };

  void BreakLine(NGLineInfo*);

  void PrepareNextLine(const NGLayoutOpportunity&, NGLineInfo*);
  NGInlineItemResult* AddItem(const NGInlineItem&,
                              unsigned end_offset,
                              NGInlineItemResults*);
  NGInlineItemResult* AddItem(const NGInlineItem&, NGInlineItemResults*);
  void ComputeCanBreakAfter(NGInlineItemResult*) const;

  void UpdatePosition(const NGInlineItemResults&);
  void ComputeLineLocation(NGLineInfo*) const;

  enum class LineBreakState {
    kDone,
    kContinue,
    kTrailing,
  };

  LineBreakState HandleText(const NGInlineItem&, LineBreakState, NGLineInfo*);
  void BreakText(NGInlineItemResult*,
                 const NGInlineItem&,
                 LayoutUnit available_width,
                 NGLineInfo*);
  LineBreakState HandleTrailingSpaces(const NGInlineItem&, NGLineInfo*);
  void AppendHyphen(const ComputedStyle&, NGLineInfo*);

  LineBreakState HandleControlItem(const NGInlineItem&,
                                   LineBreakState,
                                   NGLineInfo*);
  LineBreakState HandleAtomicInline(const NGInlineItem&, NGLineInfo*);
  void HandleFloat(const NGInlineItem&, NGInlineItemResult*);

  void HandleOpenTag(const NGInlineItem&, NGInlineItemResult*);
  void HandleCloseTag(const NGInlineItem&, NGInlineItemResults*);

  LineBreakState HandleOverflow(NGLineInfo*);
  LineBreakState HandleOverflow(NGLineInfo*,
                                LayoutUnit available_width,
                                bool force_break_anywhere);
  void Rewind(NGLineInfo*, unsigned new_end);

  void TruncateOverflowingText(NGLineInfo*);

  void SetCurrentStyle(const ComputedStyle&);
  bool IsFirstBreakOpportunity(unsigned, const NGLineInfo&) const;

  void MoveToNextOf(const NGInlineItem&);
  void MoveToNextOf(const NGInlineItemResult&);

  const String& GetText() const { return break_iterator_.GetString(); }
  bool IsFirstFormattedLine() const;
  void ComputeBaseDirection();

  LineData line_;
  NGInlineNode node_;
  NGLineBreakerMode mode_;
  const NGConstraintSpace& constraint_space_;
  Vector<NGPositionedFloat>* positioned_floats_;
  Vector<scoped_refptr<NGUnpositionedFloat>>* unpositioned_floats_;
  NGExclusionSpace* exclusion_space_;

  unsigned item_index_ = 0;
  unsigned offset_ = 0;
  bool previous_line_had_forced_break_ = false;
  LayoutUnit bfc_line_offset_;
  LayoutUnit bfc_block_offset_;
  LazyLineBreakIterator break_iterator_;
  HarfBuzzShaper shaper_;
  ShapeResultSpacing<String> spacing_;
  const Hyphenation* hyphenation_ = nullptr;

  // Keep track of handled float items. See HandleFloat().
  unsigned handled_floats_end_item_index_;

  // The current base direction for the bidi algorithm.
  // This is copied from NGInlineNode, then updated after each forced line break
  // if 'unicode-bidi: plaintext'.
  TextDirection base_direction_;

  // True when current box allows line wrapping.
  bool auto_wrap_ = false;

  bool collapse_spaces_ = false;

  // True when current box has 'word-break/word-wrap: break-word'.
  bool break_if_overflow_ = false;

  bool override_break_anywhere_ = false;

  // True when breaking at soft hyphens (U+00AD) is allowed.
  bool enable_soft_hyphen_ = true;

  // True in quirks mode or limited-quirks mode, which require line-height
  // quirks.
  // https://quirks.spec.whatwg.org/#the-line-height-calculation-quirk
  bool in_line_height_quirks_mode_ = false;

  bool ignore_floats_ = false;
};

}  // namespace blink

#endif  // NGLineBreaker_h
