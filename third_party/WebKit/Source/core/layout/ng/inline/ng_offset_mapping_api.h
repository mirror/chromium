// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGOffsetMappingAPI_h
#define NGOffsetMappingAPI_h

#include "core/layout/ng/inline/ng_inline_node.h"
#include "platform/wtf/Allocator.h"
#include "platform/wtf/Optional.h"

namespace blink {

class Node;
class NGInlineNode;
struct NGOffsetMappingUnit;

// APIs for mapping offsets between DOM and text content. See the inlined
// comments for details. Design doc: https://goo.gl/CJbxky
class NGOffsetMappingAPI {
  STATIC_ONLY(NGOffsetMappingAPI);

 public:
  // If the given Node is laid out as an inline, return the NGInlineNode that
  // encloses it. Otherwise, returns null.
  static Optional<NGInlineNode> GetNGInlineNodeFor(const Node&);

  // Maps a DOM offset in the given unit to text content offset, according to
  // the type of the unit.
  static unsigned ConvertDOMOffsetToTextContent(const NGOffsetMappingUnit&,
                                                unsigned);

  // Returns the NGOffsetMappingUnit that contains the given offset in the DOM
  // node. If there are multiple qualifying units, returns the last one.
  static const NGOffsetMappingUnit* GetMappingUnitForDOMOffset(const Node&,
                                                               unsigned);

  // Variant of the previous function with the enclosing NGInlineNode provided,
  // to avoid repeated computation.
  static const NGOffsetMappingUnit* GetMappingUnitForDOMOffset(NGInlineNode,
                                                               const Node&,
                                                               unsigned);

  // Returns the text content offset corresponding to the given DOM offset.
  static const size_t GetTextContentOffset(const Node&, unsigned);

  // TODO(xiaochengh): Implement remaining APIs.
};

}  // namespace blink

#endif  // NGOffsetMappingAPI_h
