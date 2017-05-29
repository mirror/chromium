// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WhitespaceAttacher_h
#define WhitespaceAttacher_h

#include "core/CoreExport.h"
#include "platform/heap/Member.h"

namespace blink {

class Element;
class LayoutObject;
class Text;

// Store text nodes encountered
// Reattach layout object with non-null layout object => reattach last text node
// if needed. Encounter non-null layout object => attach whitespace if needed

class CORE_EXPORT WhitespaceAttacher {
  STACK_ALLOCATED();

 public:
  WhitespaceAttacher() {}
  ~WhitespaceAttacher() { FinishSiblings(); }

  void DidVisitText(Text*);
  void DidReattachText(Text*);
  void DidVisitElement(const Element*);
  void DidReattachElement(const Element*);
  void FinishSiblings();

 private:
  void ReattachWhitespaceSiblings(LayoutObject* previous_in_flow);

  void SetLastTextNode(Text* text) {
    last_text_node_ = text;
    last_text_node_needs_reattach_ = false;
  }

  Member<Text> last_text_node_ = nullptr;
  bool last_text_node_needs_reattach_ = false;
};

}  // namespace blink

#endif  // WhitespaceAttacher_h
