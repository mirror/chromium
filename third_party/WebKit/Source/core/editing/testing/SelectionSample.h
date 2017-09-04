// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SelectionSample_h
#define SelectionSample_h

#include <string>

#include "core/editing/SelectionTemplate.h"
#include "platform/wtf/Allocator.h"
#include "platform/wtf/Forward.h"

namespace blink {

// |SelectionSample| provides parsing HTML text with selection markers and
// serializes DOM tree with selection markers.
// Selection markers are represents by "^" for selection base and "|" for
// selection extent like "assert_selection.js" in layout test.
class SelectionSample final {
  STATIC_ONLY(SelectionSample);

 public:
  // TDOO(editng-dev): We will have flat tree version of |Parse()| and
  // |SeralizeToString()| when we need.
  // Returns |SelectionInDOMTree| from |Node| and removes selection markers
  // in |CharacterData| node or removes |CharacterData| node if it contains
  // only selection markers.
  static SelectionInDOMTree Parse(Node*);

  // Note: We don't add namespace declaration if |ContainerNode| doesn't
  // have it.
  // Note: We don't escape "--" in comment.
  static std::string ToString(const ContainerNode&, const SelectionInDOMTree&);
};

}  // namespace blink

#endif  // SelectionSample_h
