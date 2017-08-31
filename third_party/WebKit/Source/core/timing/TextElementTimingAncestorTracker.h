// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TextElementTimingAncestorTracker_h
#define TextElementTimingAncestorTracker_h

#include "platform/wtf/Deque.h"
#include "platform/wtf/text/AtomicString.h"

namespace blink {

class Document;
class Element;

// This class keeps track of elements' ancestor elementtiming attributes during
// the AttachLayoutTree() process, so that when a text node is laid out, a
// PerformanceElementTiming entry will be created for all ancestor elementtiming
// names when the subsequent frame is committed.
//
// TextElementTimingAncestorTracker maintains an |element_timing_stack_| that
// tracks all valid elementtiming attributes of an element's ancestors. The
// AttachLayoutTree() process proceeds as a pre-order traversal of the DOM, and
// so as elementtiming attributes are pushed in this order, and popped after
// visiting an element's children, at any time the |element_timing_stack_|
// contains all ancestor elementtiming attributes for a given element.
//
// Since AttachLayoutTree() might not start at the root element,
// InitializeAncestorElementTimingStack must be called for the current element
// at the beginning of the process. This will initialize the stack up to and
// including the current element's parent.
class TextElementTimingAncestorTracker {
 private:
  struct Node {
    bool is_tracked;
    AtomicString element_timing_name;
  };

 public:
  // This class handles pushing elementtiming name attributes onto and off of
  // TextElementTimingAncestorTracker's |element_timing_stack_|. The current
  // Element's elementtiming attribute should be pushed onto the stack right
  // before calling AttachLayoutTree() for each child, and popped right after,
  // which is handled by this class during construction and destruction.
  class ScopedElementTimingNameTracker {
   public:
    ScopedElementTimingNameTracker(Element*, TextElementTimingAncestorTracker*);
    ~ScopedElementTimingNameTracker();

   private:
    size_t initial_stack_size_;
    bool has_element_timing_;
    TextElementTimingAncestorTracker* element_timing_ancestor_tracker_;
  };

  TextElementTimingAncestorTracker();
  ~TextElementTimingAncestorTracker();

  // Track all ancestor elementtiming attributes in the stack if they are not
  // currently tracked. This is called during AttachLayoutTree() for Text nodes,
  // and will cause a PerformanceEntryTiming entry to be created for each
  // ancestor elementtiming attribute when the subsequent frame commits.
  void TrackAllElementsIfNeeded(Document&);

  // For a given |element|, create the |element_timing_stack_| for up to the
  // root node by pushing all elementtiming attributes onto the stack for all of
  // its ancestors.
  void InitializeAncestorElementTimingStack(const Element*);

 private:
  // Push an elementtiming name attribute onto the |element_timing_stack_|. This
  // should be called for an element before its children.
  void PushElementTimingName(const AtomicString& name);

  // Pop an elementtiming name attribute off of the |element_timing_stack_|.
  // This should be called for an element after all of its children.
  void PopElementTimingName();

  size_t StackSize() const;

  // |element_timing_stack_| keeps track of ancestor elementtiming attributes,
  // with the oldest ancestor's attribute stored in the front.
  Deque<Node> element_timing_stack_;
};

}  // namespace blink

#endif
