// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EditingCommandsUtilities_h
#define EditingCommandsUtilities_h

#include "core/CoreExport.h"
#include "core/editing/Forward.h"
#include "platform/wtf/text/CharacterNames.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {

class Document;
class Element;
class HTMLElement;
class Node;
class QualifiedName;

// -------------------------------------------------------------------------
// Node
// -------------------------------------------------------------------------

Node* HighestNodeToRemoveInPruning(Node*, const Node* exclude_node = nullptr);

Element* EnclosingTableCell(const Position&);
Node* EnclosingEmptyListItem(const VisiblePosition&);

bool IsTableStructureNode(const Node*);
bool IsNodeRendered(const Node&);
// Returns true if specified nodes are elements, have identical tag names,
// have identical attributes, and are editable.
CORE_EXPORT bool AreIdenticalElements(const Node&, const Node&);

// -------------------------------------------------------------------------
// Position
// -------------------------------------------------------------------------

Position PositionBeforeContainingSpecialElement(
    const Position&,
    HTMLElement** containing_special_element = nullptr);
Position PositionAfterContainingSpecialElement(
    const Position&,
    HTMLElement** containing_special_element = nullptr);

bool LineBreakExistsAtPosition(const Position&);

// miscellaneous functions on Position

enum WhitespacePositionOption {
  kNotConsiderNonCollapsibleWhitespace,
  kConsiderNonCollapsibleWhitespace
};

// |leadingCollapsibleWhitespacePosition(position)| returns a previous position
// of |position| if it is at collapsible whitespace, otherwise it returns null
// position. When it is called with |NotConsiderNonCollapsibleWhitespace| and
// a previous position in a element which has CSS property "white-space:pre",
// or its variant, |leadingCollapsibleWhitespacePosition()| returns null
// position.
Position LeadingCollapsibleWhitespacePosition(
    const Position&,
    TextAffinity,
    WhitespacePositionOption = kNotConsiderNonCollapsibleWhitespace);

// TDOO(editing-dev): We should move |TrailingWhitespacePosition()| to
// "DeleteSelection.cpp" since it is used only in "DeleteSelection.cpp".
Position TrailingWhitespacePosition(
    const Position&,
    WhitespacePositionOption = kNotConsiderNonCollapsibleWhitespace);

unsigned NumEnclosingMailBlockquotes(const Position&);

// -------------------------------------------------------------------------
// VisiblePosition
// -------------------------------------------------------------------------

bool LineBreakExistsAtVisiblePosition(const VisiblePosition&);

// -------------------------------------------------------------------------
// HTMLElement
// -------------------------------------------------------------------------

HTMLElement* CreateHTMLElement(Document&, const QualifiedName&);
HTMLElement* EnclosingList(const Node*);
HTMLElement* OutermostEnclosingList(const Node*,
                                    const HTMLElement* root_list = nullptr);
Node* EnclosingListChild(const Node*);

// -------------------------------------------------------------------------
// Element
// -------------------------------------------------------------------------

bool CanMergeLists(const Element& first_list, const Element& second_list);

// -------------------------------------------------------------------------
// VisibleSelection
// -------------------------------------------------------------------------

// Functions returning VisibleSelection
VisibleSelection SelectionForParagraphIteration(const VisibleSelection&);

const String& NonBreakingSpaceString();

}  // namespace blink

#endif
