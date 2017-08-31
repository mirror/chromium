// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TextElementTiming_h
#define TextElementTiming_h

#include "core/dom/Document.h"
#include "platform/Supplementable.h"
#include "platform/wtf/HashSet.h"
#include "platform/wtf/Noncopyable.h"
#include "platform/wtf/text/AtomicString.h"

namespace blink {

class LocalFrame;
class Performance;

// TextElementTiming is responsible for tracking text elementtimings for a given
// document.
//
// For elements with text, PerformanceElementTiming entries will be created for:
//   1) The element itself if it has an elementtiming attribute.
//   2) Any of its ancestors that have an elementtiming attribute.
// Only one entry is created per elementtiming attribute.
//
// During the AttachLayoutTree() process, the elementtiming attributes for
// elements meeting the above criteria are queued, and PerformanceElementTiming
// entries are created for any pending |element_timing_entry_names_| when the
// subsequent frame commits.
class CORE_EXPORT TextElementTiming final
    : public GarbageCollectedFinalized<TextElementTiming>,
      public Supplement<Document> {
  WTF_MAKE_NONCOPYABLE(TextElementTiming);
  USING_GARBAGE_COLLECTED_MIXIN(TextElementTiming);

 public:
  virtual ~TextElementTiming();

  static TextElementTiming& From(Document&);

  // Track an |element_timing_name| if
  // (1) It isn't null or an empty string, and
  // (2) We haven't dispatched or queued a performance entry for this
  // elementtiming name.
  void TrackElementTimingNameIfNeeded(const AtomicString& element_timing_name);

  // Notification from the renderer that the frame was committed. We use this
  // signal to create any pending performance entries, and use this commit
  // timestamp for the entry.
  void DidCommitCompositorFrame();

  static bool IsValidElementTimingName(const AtomicString& element_timing_name);

  DECLARE_VIRTUAL_TRACE();

 private:
  friend class TextElementTimingTest;

  explicit TextElementTiming(Document&);
  LocalFrame* GetFrame() const;

  void CreatePendingTextElementTimingEntries(double timestamp);

  static Performance* GetPerformanceInstance(LocalFrame*);

  HashSet<AtomicString> element_timing_entry_names_;
  HashSet<AtomicString> pending_entry_names_;
};

}  // namespace blink

#endif
