// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TextElementTiming_h
#define TextElementTiming_h

#include <memory>

#include "core/dom/Document.h"
#include "platform/Supplementable.h"
#include "platform/wtf/HashMap.h"
#include "platform/wtf/HashSet.h"
#include "platform/wtf/Noncopyable.h"

namespace blink {

class LocalFrame;

// TextElementTiming is responsible for tracking text element display timings
// for a given document.
class CORE_EXPORT TextElementTiming final
    : public GarbageCollectedFinalized<TextElementTiming>,
      public Supplement<Document> {
  WTF_MAKE_NONCOPYABLE(TextElementTiming);
  USING_GARBAGE_COLLECTED_MIXIN(TextElementTiming);

 private:
  class TimingEntry {
   public:
    TimingEntry(const AtomicString& timing_id, double timestamp);

    AtomicString TimingId() const { return timing_id_; }
    double Timestamp() const { return timestamp_; }

   private:
    AtomicString timing_id_;
    double timestamp_;
  };

 public:
  virtual ~TextElementTiming();

  static TextElementTiming& From(Document&);

  // Track a Text element if:
  // (1) The corresponding DOM node has its elementtiming attribute set.
  // (2) We don't already have a performance entry for its elementtiming id.
  void TrackTextElementIfNeeded(Text*);

  // Notification from the renderer that the frame was committed. We use this
  // signal to create any pending performance entries, and use this commit
  // timetamp for the entry.
  void DidCommitCompositorFrame();

  DECLARE_VIRTUAL_TRACE();

 private:
  friend class TextElementTimingTest;

  explicit TextElementTiming(Document&);
  LocalFrame* GetFrame() const;

  void CreatePendingTextElementTimingEntries(double timestamp);

  HashMap<AtomicString, std::unique_ptr<TimingEntry>> timing_entries_;
  HashSet<AtomicString> pending_entry_ids_;
};

}  // namespace blink

#endif
