/*
 * Copyright (C) 2004 Apple Computer, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef VisibleUnits_h
#define VisibleUnits_h

#include "core/CoreExport.h"
#include "core/editing/EditingBoundary.h"
#include "core/editing/EphemeralRange.h"
#include "core/editing/PositionWithAffinity.h"
#include "core/editing/VisiblePosition.h"
#include "platform/text/TextDirection.h"

namespace blink {

class LayoutRect;
class LayoutUnit;
class LayoutObject;
class Node;
class IntPoint;
class InlineBox;
class LocalFrame;

enum EWordSide { kRightWordIfOnBoundary = false, kLeftWordIfOnBoundary = true };

struct InlineBoxPosition {
  InlineBox* inline_box;
  int offset_in_box;

  InlineBoxPosition() : inline_box(nullptr), offset_in_box(0) {}

  InlineBoxPosition(InlineBox* inline_box, int offset_in_box)
      : inline_box(inline_box), offset_in_box(offset_in_box) {}

  bool operator==(const InlineBoxPosition& other) const {
    return inline_box == other.inline_box &&
           offset_in_box == other.offset_in_box;
  }

  bool operator!=(const InlineBoxPosition& other) const {
    return !operator==(other);
  }
};

// The print for |InlineBoxPosition| is available only for testing
// in "webkit_unit_tests", and implemented in
// "core/editing/VisibleUnitsTest.cpp".
std::ostream& operator<<(std::ostream&, const InlineBoxPosition&);

CORE_EXPORT_N1827 LayoutObject* AssociatedLayoutObjectOf(const Node&,
                                                   int offset_in_node);

// offset functions on Node
CORE_EXPORT_N1828 int CaretMinOffset(const Node*);
CORE_EXPORT_N1829 int CaretMaxOffset(const Node*);

// Position
// mostForward/BackwardCaretPosition are used for moving back and forth between
// visually equivalent candidates.
// For example, for the text node "foo     bar" where whitespace is
// collapsible, there are two candidates that map to the VisiblePosition
// between 'b' and the space, after first space and before last space.
//
// mostBackwardCaretPosition returns the left candidate and also returs
// [boundary, 0] for any of the positions from [boundary, 0] to the first
// candidate in boundary, where
// endsOfNodeAreVisuallyDistinctPositions(boundary) is true.
//
// mostForwardCaretPosition() returns the right one and also returns the
// last position in the last atomic node in boundary for all of the positions
// in boundary after the last candidate, where
// endsOfNodeAreVisuallyDistinctPositions(boundary).
// FIXME: This function should never be called when the line box tree is dirty.
// See https://bugs.webkit.org/show_bug.cgi?id=97264
CORE_EXPORT_N1830 Position MostBackwardCaretPosition(
    const Position&,
    EditingBoundaryCrossingRule = kCannotCrossEditingBoundary);
CORE_EXPORT_N1831 PositionInFlatTree MostBackwardCaretPosition(
    const PositionInFlatTree&,
    EditingBoundaryCrossingRule = kCannotCrossEditingBoundary);
CORE_EXPORT_N1832 Position MostForwardCaretPosition(
    const Position&,
    EditingBoundaryCrossingRule = kCannotCrossEditingBoundary);
CORE_EXPORT_N1833 PositionInFlatTree MostForwardCaretPosition(
    const PositionInFlatTree&,
    EditingBoundaryCrossingRule = kCannotCrossEditingBoundary);

CORE_EXPORT_N1834 bool IsVisuallyEquivalentCandidate(const Position&);
CORE_EXPORT_N1835 bool IsVisuallyEquivalentCandidate(const PositionInFlatTree&);

// Whether or not [node, 0] and [node, lastOffsetForEditing(node)] are their own
// VisiblePositions.
// If true, adjacent candidates are visually distinct.
CORE_EXPORT_N1836 bool EndsOfNodeAreVisuallyDistinctPositions(const Node*);

CORE_EXPORT_N1837 Position CanonicalPositionOf(const Position&);
CORE_EXPORT_N1838 PositionInFlatTree CanonicalPositionOf(const PositionInFlatTree&);

// Bounds of (possibly transformed) caret in absolute coords
CORE_EXPORT_N1839 IntRect AbsoluteCaretBoundsOf(const VisiblePosition&);
CORE_EXPORT_N1840 IntRect AbsoluteCaretBoundsOf(const VisiblePositionInFlatTree&);

CORE_EXPORT_N1841 IntRect AbsoluteSelectionBoundsOf(const VisiblePosition&);
CORE_EXPORT_N1842 IntRect AbsoluteSelectionBoundsOf(const VisiblePositionInFlatTree&);

CORE_EXPORT_N1843 UChar32 CharacterAfter(const VisiblePosition&);
CORE_EXPORT_N1844 UChar32 CharacterAfter(const VisiblePositionInFlatTree&);
CORE_EXPORT_N1845 UChar32 CharacterBefore(const VisiblePosition&);
CORE_EXPORT_N1846 UChar32 CharacterBefore(const VisiblePositionInFlatTree&);

// TODO(yosin) Since return value of |leftPositionOf()| with |VisiblePosition|
// isn't defined well on flat tree, we should not use it for a position in
// flat tree.
CORE_EXPORT_N1847 VisiblePosition LeftPositionOf(const VisiblePosition&);
CORE_EXPORT_N1848 VisiblePositionInFlatTree
LeftPositionOf(const VisiblePositionInFlatTree&);
// TODO(yosin) Since return value of |rightPositionOf()| with |VisiblePosition|
// isn't defined well on flat tree, we should not use it for a position in
// flat tree.
CORE_EXPORT_N1849 VisiblePosition RightPositionOf(const VisiblePosition&);
CORE_EXPORT_N1850 VisiblePositionInFlatTree
RightPositionOf(const VisiblePositionInFlatTree&);

CORE_EXPORT_N1851 VisiblePosition
NextPositionOf(const VisiblePosition&,
               EditingBoundaryCrossingRule = kCanCrossEditingBoundary);
CORE_EXPORT_N1852 VisiblePositionInFlatTree
NextPositionOf(const VisiblePositionInFlatTree&,
               EditingBoundaryCrossingRule = kCanCrossEditingBoundary);
CORE_EXPORT_N1853 VisiblePosition
PreviousPositionOf(const VisiblePosition&,
                   EditingBoundaryCrossingRule = kCanCrossEditingBoundary);
CORE_EXPORT_N1854 VisiblePositionInFlatTree
PreviousPositionOf(const VisiblePositionInFlatTree&,
                   EditingBoundaryCrossingRule = kCanCrossEditingBoundary);

// words
// TODO(yoichio): Replace |startOfWord| to |startOfWordPosition| because
// returned Position should be canonicalized with |previousBoundary()| by
// TextItetator.
CORE_EXPORT_N1855 Position StartOfWordPosition(const VisiblePosition&,
                                         EWordSide = kRightWordIfOnBoundary);
CORE_EXPORT_N1856 VisiblePosition StartOfWord(const VisiblePosition&,
                                        EWordSide = kRightWordIfOnBoundary);
CORE_EXPORT_N1857 PositionInFlatTree
StartOfWordPosition(const VisiblePositionInFlatTree&,
                    EWordSide = kRightWordIfOnBoundary);
CORE_EXPORT_N1858 VisiblePositionInFlatTree
StartOfWord(const VisiblePositionInFlatTree&,
            EWordSide = kRightWordIfOnBoundary);
// TODO(yoichio): Replace |endOfWord| to |endOfWordPosition| because returned
// Position should be canonicalized with |nextBoundary()| by TextItetator.
CORE_EXPORT_N1859 Position EndOfWordPosition(const VisiblePosition&,
                                       EWordSide = kRightWordIfOnBoundary);
CORE_EXPORT_N1860 VisiblePosition EndOfWord(const VisiblePosition&,
                                      EWordSide = kRightWordIfOnBoundary);
CORE_EXPORT_N1861 PositionInFlatTree
EndOfWordPosition(const VisiblePositionInFlatTree&,
                  EWordSide = kRightWordIfOnBoundary);
CORE_EXPORT_N1862 VisiblePositionInFlatTree
EndOfWord(const VisiblePositionInFlatTree&, EWordSide = kRightWordIfOnBoundary);
VisiblePosition PreviousWordPosition(const VisiblePosition&);
VisiblePosition NextWordPosition(const VisiblePosition&);
VisiblePosition RightWordPosition(const VisiblePosition&,
                                  bool skips_space_when_moving_right);
VisiblePosition LeftWordPosition(const VisiblePosition&,
                                 bool skips_space_when_moving_right);

// sentences
CORE_EXPORT_N1863 VisiblePosition StartOfSentence(const VisiblePosition&);
CORE_EXPORT_N1864 VisiblePositionInFlatTree
StartOfSentence(const VisiblePositionInFlatTree&);
CORE_EXPORT_N1865 VisiblePosition EndOfSentence(const VisiblePosition&);
CORE_EXPORT_N1866 VisiblePositionInFlatTree
EndOfSentence(const VisiblePositionInFlatTree&);
VisiblePosition PreviousSentencePosition(const VisiblePosition&);
VisiblePosition NextSentencePosition(const VisiblePosition&);
EphemeralRange ExpandEndToSentenceBoundary(const EphemeralRange&);
EphemeralRange ExpandRangeToSentenceBoundary(const EphemeralRange&);

// lines
// TODO(yosin) Return values of |VisiblePosition| version of |startOfLine()|
// with shadow tree isn't defined well. We should not use it for shadow tree.
CORE_EXPORT_N1867 VisiblePosition StartOfLine(const VisiblePosition&);
CORE_EXPORT_N1868 VisiblePositionInFlatTree
StartOfLine(const VisiblePositionInFlatTree&);
// TODO(yosin) Return values of |VisiblePosition| version of |endOfLine()| with
// shadow tree isn't defined well. We should not use it for shadow tree.
CORE_EXPORT_N1869 VisiblePosition EndOfLine(const VisiblePosition&);
CORE_EXPORT_N1870 VisiblePositionInFlatTree
EndOfLine(const VisiblePositionInFlatTree&);
enum EditableType { kContentIsEditable, kHasEditableAXRole };
CORE_EXPORT_N1871 VisiblePosition
PreviousLinePosition(const VisiblePosition&,
                     LayoutUnit line_direction_point,
                     EditableType = kContentIsEditable);
CORE_EXPORT_N1872 VisiblePosition NextLinePosition(const VisiblePosition&,
                                             LayoutUnit line_direction_point,
                                             EditableType = kContentIsEditable);
CORE_EXPORT_N1873 bool InSameLine(const VisiblePosition&, const VisiblePosition&);
CORE_EXPORT_N1874 bool InSameLine(const VisiblePositionInFlatTree&,
                            const VisiblePositionInFlatTree&);
CORE_EXPORT_N1875 bool InSameLine(const PositionWithAffinity&,
                            const PositionWithAffinity&);
CORE_EXPORT_N1876 bool InSameLine(const PositionInFlatTreeWithAffinity&,
                            const PositionInFlatTreeWithAffinity&);
CORE_EXPORT_N1877 bool IsStartOfLine(const VisiblePosition&);
CORE_EXPORT_N1878 bool IsStartOfLine(const VisiblePositionInFlatTree&);
CORE_EXPORT_N1879 bool IsEndOfLine(const VisiblePosition&);
CORE_EXPORT_N1880 bool IsEndOfLine(const VisiblePositionInFlatTree&);
// TODO(yosin) Return values of |VisiblePosition| version of
// |logicalStartOfLine()| with shadow tree isn't defined well. We should not use
// it for shadow tree.
CORE_EXPORT_N1881 VisiblePosition LogicalStartOfLine(const VisiblePosition&);
CORE_EXPORT_N1882 VisiblePositionInFlatTree
LogicalStartOfLine(const VisiblePositionInFlatTree&);
// TODO(yosin) Return values of |VisiblePosition| version of
// |logicalEndOfLine()| with shadow tree isn't defined well. We should not use
// it for shadow tree.
CORE_EXPORT_N1883 VisiblePosition LogicalEndOfLine(const VisiblePosition&);
CORE_EXPORT_N1884 VisiblePositionInFlatTree
LogicalEndOfLine(const VisiblePositionInFlatTree&);
CORE_EXPORT_N1885 bool IsLogicalEndOfLine(const VisiblePosition&);
CORE_EXPORT_N1886 bool IsLogicalEndOfLine(const VisiblePositionInFlatTree&);
VisiblePosition LeftBoundaryOfLine(const VisiblePosition&, TextDirection);
VisiblePosition RightBoundaryOfLine(const VisiblePosition&, TextDirection);

// paragraphs (perhaps a misnomer, can be divided by line break elements)
// TODO(yosin) Since return value of |startOfParagraph()| with |VisiblePosition|
// isn't defined well on flat tree, we should not use it for a position in
// flat tree.
CORE_EXPORT_N1887 VisiblePosition
StartOfParagraph(const VisiblePosition&,
                 EditingBoundaryCrossingRule = kCannotCrossEditingBoundary);
CORE_EXPORT_N1888 VisiblePositionInFlatTree
StartOfParagraph(const VisiblePositionInFlatTree&,
                 EditingBoundaryCrossingRule = kCannotCrossEditingBoundary);
CORE_EXPORT_N1889 VisiblePosition
EndOfParagraph(const VisiblePosition&,
               EditingBoundaryCrossingRule = kCannotCrossEditingBoundary);
CORE_EXPORT_N1890 VisiblePositionInFlatTree
EndOfParagraph(const VisiblePositionInFlatTree&,
               EditingBoundaryCrossingRule = kCannotCrossEditingBoundary);
VisiblePosition StartOfNextParagraph(const VisiblePosition&);
VisiblePosition PreviousParagraphPosition(const VisiblePosition&, LayoutUnit x);
VisiblePosition NextParagraphPosition(const VisiblePosition&, LayoutUnit x);
CORE_EXPORT_N1891 bool IsStartOfParagraph(
    const VisiblePosition&,
    EditingBoundaryCrossingRule = kCannotCrossEditingBoundary);
CORE_EXPORT_N1892 bool IsStartOfParagraph(
    const VisiblePositionInFlatTree&,
    EditingBoundaryCrossingRule = kCannotCrossEditingBoundary);
CORE_EXPORT_N1893 bool IsEndOfParagraph(
    const VisiblePosition&,
    EditingBoundaryCrossingRule = kCannotCrossEditingBoundary);
CORE_EXPORT_N1894 bool IsEndOfParagraph(
    const VisiblePositionInFlatTree&,
    EditingBoundaryCrossingRule = kCannotCrossEditingBoundary);
bool InSameParagraph(const VisiblePosition&,
                     const VisiblePosition&,
                     EditingBoundaryCrossingRule = kCannotCrossEditingBoundary);
EphemeralRange ExpandToParagraphBoundary(const EphemeralRange&);

// blocks (true paragraphs; line break elements don't break blocks)
VisiblePosition StartOfBlock(
    const VisiblePosition&,
    EditingBoundaryCrossingRule = kCannotCrossEditingBoundary);
VisiblePosition EndOfBlock(
    const VisiblePosition&,
    EditingBoundaryCrossingRule = kCannotCrossEditingBoundary);
bool InSameBlock(const VisiblePosition&, const VisiblePosition&);
bool IsStartOfBlock(const VisiblePosition&);
bool IsEndOfBlock(const VisiblePosition&);

// document
CORE_EXPORT_N1895 VisiblePosition StartOfDocument(const VisiblePosition&);
CORE_EXPORT_N1896 VisiblePositionInFlatTree
StartOfDocument(const VisiblePositionInFlatTree&);
CORE_EXPORT_N1897 VisiblePosition EndOfDocument(const VisiblePosition&);
CORE_EXPORT_N1898 VisiblePositionInFlatTree
EndOfDocument(const VisiblePositionInFlatTree&);
bool IsStartOfDocument(const VisiblePosition&);
bool IsEndOfDocument(const VisiblePosition&);

// editable content
VisiblePosition StartOfEditableContent(const VisiblePosition&);
VisiblePosition EndOfEditableContent(const VisiblePosition&);
CORE_EXPORT_N1899 bool IsEndOfEditableOrNonEditableContent(const VisiblePosition&);
CORE_EXPORT_N1900 bool IsEndOfEditableOrNonEditableContent(
    const VisiblePositionInFlatTree&);

CORE_EXPORT_N1901 InlineBoxPosition ComputeInlineBoxPosition(const Position&,
                                                       TextAffinity);
CORE_EXPORT_N1902 InlineBoxPosition
ComputeInlineBoxPosition(const Position&,
                         TextAffinity,
                         TextDirection primary_direction);
CORE_EXPORT_N1903 InlineBoxPosition
ComputeInlineBoxPosition(const PositionInFlatTree&, TextAffinity);
CORE_EXPORT_N1904 InlineBoxPosition
ComputeInlineBoxPosition(const PositionInFlatTree&,
                         TextAffinity,
                         TextDirection primary_direction);
CORE_EXPORT_N1905 InlineBoxPosition ComputeInlineBoxPosition(const VisiblePosition&);

// Rect is local to the returned layoutObject
CORE_EXPORT_N1906 LayoutRect LocalCaretRectOfPosition(const PositionWithAffinity&,
                                                LayoutObject*&);
CORE_EXPORT_N1907 LayoutRect
LocalCaretRectOfPosition(const PositionInFlatTreeWithAffinity&, LayoutObject*&);
bool HasRenderedNonAnonymousDescendantsWithHeight(LayoutObject*);

// Returns a hit-tested VisiblePosition for the given point in contents-space
// coordinates.
CORE_EXPORT_N1908 VisiblePosition VisiblePositionForContentsPoint(const IntPoint&,
                                                            LocalFrame*);

CORE_EXPORT_N1909 bool RendersInDifferentPosition(const Position&, const Position&);

CORE_EXPORT_N1910 Position SkipWhitespace(const Position&);
CORE_EXPORT_N1911 PositionInFlatTree SkipWhitespace(const PositionInFlatTree&);

CORE_EXPORT_N1912 IntRect ComputeTextRect(const EphemeralRange&);
IntRect ComputeTextRect(const EphemeralRangeInFlatTree&);
FloatRect ComputeTextFloatRect(const EphemeralRange&);

}  // namespace blink

#endif  // VisibleUnits_h
