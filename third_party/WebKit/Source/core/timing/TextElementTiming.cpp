// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/timing/TextElementTiming.h"

#include <memory>
#include <utility>

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

TextElementTiming::TimingEntry::TimingEntry(const AtomicString& timing_id,
                                            double timestamp)
    : timing_id_(timing_id), timestamp_(timestamp) {}

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
  AtomicString timing_id =
      text->parentElement()->getAttribute(HTMLNames::elementtimingAttr);
  if (!timing_id || timing_id.IsEmpty() || timing_entries_.Contains(timing_id))
    return;
  pending_entry_ids_.insert(timing_id);
}

void TextElementTiming::DidCommitCompositorFrame() {
  if (pending_entry_ids_.IsEmpty())
    return;
  CreatePendingTextElementTimingEntries(MonotonicallyIncreasingTime());
}

void TextElementTiming::CreatePendingTextElementTimingEntries(
    double timestamp) {
  Performance* performance = GetPerformanceInstance(GetFrame());

  for (const auto& timing_id : pending_entry_ids_) {
    DCHECK(!timing_entries_.Contains(timing_id));
    // TODO(shaseley): Remove?
    TRACE_EVENT_INSTANT_WITH_TIMESTAMP1(
        "loading,rail,devtools.timeline", "TextElementTiming",
        TRACE_EVENT_SCOPE_PROCESS, TraceEvent::ToTraceTimestamp(timestamp),
        "timing_id", timing_id.Utf8());
    timing_entries_.insert(timing_id,
                           WTF::MakeUnique<TimingEntry>(timing_id, timestamp));
    if (performance)
      performance->AddElementTiming(timing_id.GetString(), timestamp);
  }
  pending_entry_ids_.clear();
}

}  // namespace blink
