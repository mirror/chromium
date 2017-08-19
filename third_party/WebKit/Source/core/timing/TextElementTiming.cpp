// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/timing/TextElementTiming.h"

#include "core/HTMLNames.h"
#include "core/dom/Document.h"
#include "core/dom/Element.h"
#include "core/frame/LocalDOMWindow.h"
#include "core/frame/LocalFrame.h"
#include "core/frame/LocalFrameView.h"
#include "core/timing/DOMWindowPerformance.h"
#include "core/timing/Performance.h"
#include "platform/Histogram.h"
#include "platform/RuntimeEnabledFeatures.h"
#include "platform/instrumentation/tracing/TraceEvent.h"
#include "platform/wtf/CurrentTime.h"
#include "platform/wtf/PtrUtil.h"

namespace blink {

namespace {

Performance* GetPerformanceInstance(LocalFrame* frame) {
  Performance* performance = nullptr;
  if (frame && frame->DomWindow()) {
    performance = DOMWindowPerformance::performance(*frame->DomWindow());
  }
  return performance;
}

}  // namespace

static const char kSupplementName[] = "TextElementTiming";

// Static
TextElementTiming& TextElementTiming::From(Document& document) {
  TextElementTiming* timing = static_cast<TextElementTiming*>(
      Supplement<Document>::From(document, kSupplementName));
  if (!timing) {
    timing = new TextElementTiming(document);
    Supplement<Document>::ProvideTo(document, kSupplementName, timing);
  }
  return *timing;
}

DEFINE_TRACE(TextElementTiming) {
  Supplement<Document>::Trace(visitor);
}

TextElementTiming::TextElementTiming(Document& document)
    : Supplement<Document>(document) {}

TextElementTiming::~TextElementTiming() = default;

LocalFrame* TextElementTiming::GetFrame() const {
  return GetSupplementable()->GetFrame();
}

void TextElementTiming::TrackTextElementIfNeeded(Text* text) {
  if (!RuntimeEnabledFeatures::PerformanceElementTimingEnabled())
    return;
  DCHECK(text);
  if (!text->parentElement())
    return;
  AtomicString element_timing_name =
      text->parentElement()->getAttribute(HTMLNames::elementtimingAttr);
  if (!element_timing_name || element_timing_name.IsEmpty() ||
      element_timing_entry_names_.Contains(element_timing_name)) {
    return;
  }
  pending_entry_names_.insert(element_timing_name);
}

void TextElementTiming::DidCommitCompositorFrame() {
  if (pending_entry_names_.IsEmpty())
    return;
  CreatePendingTextElementTimingEntries(MonotonicallyIncreasingTime());
}

void TextElementTiming::CreatePendingTextElementTimingEntries(
    double timestamp) {
  Performance* performance = GetPerformanceInstance(GetFrame());
  if (!performance)
    return;

  for (const auto& element_timing_name : pending_entry_names_) {
    DCHECK(!element_timing_entry_names_.Contains(element_timing_name));
    TRACE_EVENT_INSTANT_WITH_TIMESTAMP1(
        "loading,rail,devtools.timeline", "TextElementTiming",
        TRACE_EVENT_SCOPE_PROCESS, TraceEvent::ToTraceTimestamp(timestamp),
        "element_timing_name", element_timing_name.Utf8());
    element_timing_entry_names_.insert(element_timing_name);
    performance->AddTextElementTiming(element_timing_name.GetString(),
                                      timestamp);
  }
  pending_entry_names_.clear();
}

}  // namespace blink
