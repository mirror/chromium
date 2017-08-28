// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/timing/TextElementTimingAncestorTracker.h"

#include "core/dom/Element.h"
#include "core/dom/ElementTraversal.h"
#include "core/timing/TextElementTiming.h"
#include "platform/RuntimeEnabledFeatures.h"

namespace blink {

TextElementTimingAncestorTracker::ScopedElementTimingNameTracker::
    ScopedElementTimingNameTracker(
        Element* element,
        RefPtr<TextElementTimingAncestorTracker> attach_layout_helper)
    : initial_stack_size_(0),
      has_element_timing_(false),
      attach_layout_helper_(attach_layout_helper) {
  DCHECK(attach_layout_helper);

  if (!RuntimeEnabledFeatures::PerformanceElementTimingEnabled())
    return;

  initial_stack_size_ = attach_layout_helper_->StackSize();

  const AtomicString& element_timing_name =
      element->FastGetAttribute(HTMLNames::elementtimingAttr);
  if (!TextElementTiming::IsValidElementTimingName(element_timing_name))
    return;
  has_element_timing_ = true;
  attach_layout_helper_->PushElementTimingName(element_timing_name);
}

TextElementTimingAncestorTracker::ScopedElementTimingNameTracker::
    ~ScopedElementTimingNameTracker() {
  if (!RuntimeEnabledFeatures::PerformanceElementTimingEnabled())
    return;

  if (has_element_timing_)
    attach_layout_helper_->PopElementTimingName();
  DCHECK_EQ(initial_stack_size_, attach_layout_helper_->StackSize());
}

TextElementTimingAncestorTracker::TextElementTimingAncestorTracker() = default;

TextElementTimingAncestorTracker::~TextElementTimingAncestorTracker() = default;

void TextElementTimingAncestorTracker::PushElementTimingName(
    const AtomicString& name) {
  element_timing_stack_.push_front(Node{false, name});
}

void TextElementTimingAncestorTracker::PopElementTimingName() {
  DCHECK(!element_timing_stack_.empty());
  element_timing_stack_.pop_front();
}

void TextElementTimingAncestorTracker::TrackAllElementsIfNeeded(
    TextElementTiming& timing) {
  if (!RuntimeEnabledFeatures::PerformanceElementTimingEnabled())
    return;

  for (auto& node : element_timing_stack_) {
    if (node.is_tracked)
      return;
    timing.TrackElementTimingNameIfNeeded(node.element_timing_name);
    node.is_tracked = true;
  }
}

void TextElementTimingAncestorTracker::InitializeAncestorElementTimingStack(
    const Element* element) {
  if (!RuntimeEnabledFeatures::PerformanceElementTimingEnabled())
    return;

  DCHECK(element_timing_stack_.empty());

  Element* parent = Traversal<Element>::FirstAncestor(*element);

  for (element = parent; element;
       element = Traversal<Element>::FirstAncestor(*element)) {
    const AtomicString& element_timing_name =
        element->FastGetAttribute(HTMLNames::elementtimingAttr);
    // We're building the stack bottom up, so push_back so the oldest
    // ancestor's attribute is in the front.
    if (!element_timing_name.IsNull() && !element_timing_name.IsEmpty())
      element_timing_stack_.push_back(Node{false, element_timing_name});
  }
}

size_t TextElementTimingAncestorTracker::StackSize() const {
  return element_timing_stack_.size();
}

}  // namespace blink
