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

// TextElementTiming is responsible for tracking text elementtimings for a given
// document.
//
// For elements with text, a PerformanceElementTiming entry will be created for
// each elementtiming attribute name for that element and all of its ancestors.
// The elementtiming attributes are queued during the AttachLayoutTree()
// process, and entries are created for any pending
// |element_timing_entry_names_| when the subsequent frame commits.
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
  // (2) We don't already have, or will create a performance entry for its
  // elementtiming name.
  void TrackElementTimingNameIfNeeded(const AtomicString& element_timing_name);

  // Notification from the renderer that the frame was committed. We use this
  // signal to create any pending performance entries, and use this commit
  // timetamp for the entry.
  void DidCommitCompositorFrame();

  static bool IsValidElementTimingName(const AtomicString& element_timing_name);

  DECLARE_VIRTUAL_TRACE();

 private:
  friend class TextElementTimingTest;

  explicit TextElementTiming(Document&);
  LocalFrame* GetFrame() const;

  void CreatePendingTextElementTimingEntries(double timestamp);

  HashSet<AtomicString> element_timing_entry_names_;
  HashSet<AtomicString> pending_entry_names_;
};

}  // namespace blink

#endif
