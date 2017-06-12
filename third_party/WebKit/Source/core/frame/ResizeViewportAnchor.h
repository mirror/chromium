// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ResizeViewportAnchor_h
#define ResizeViewportAnchor_h

#include "core/CoreExport.h"
#include "core/page/Page.h"
#include "platform/heap/Handle.h"
#include "platform/scroll/ScrollTypes.h"

namespace blink {

class LocalFrameView;

// This class scrolls the viewports to compensate for bounds clamping caused by
// viewport size changes.
//
// It is needed when the layout viewport grows (causing its own scroll position
// to be clamped) and also when it shrinks (causing the visual viewport's scroll
// position to be clamped).
class CORE_EXPORT ResizeViewportAnchor final
    : public GarbageCollected<ResizeViewportAnchor> {
  WTF_MAKE_NONCOPYABLE(ResizeViewportAnchor);

 public:
  ResizeViewportAnchor(Page& page)
      : page_(page), resize_scope_count_(0), clamping_scope_count_(0) {}

  class ResizeScope {
    STACK_ALLOCATED();

   public:
    explicit ResizeScope(ResizeViewportAnchor& anchor) : anchor_(anchor) {
      anchor_->BeginResizeScope();
    }
    ~ResizeScope() { anchor_->EndResizeScope(); }

   private:
    Member<ResizeViewportAnchor> anchor_;
  };

  class ClampingScope {
    DISALLOW_NEW_EXCEPT_PLACEMENT_NEW();

   public:
    explicit ClampingScope(ResizeViewportAnchor& anchor) : anchor_(anchor) {
      anchor_->BeginClampingScope();
    }
    ~ClampingScope() { anchor_->EndClampingScope(); }

    DEFINE_INLINE_TRACE() { visitor->Trace(anchor_); }

   private:
    Member<ResizeViewportAnchor> anchor_;
  };

  void ResizeFrameView(const IntSize&);

  DEFINE_INLINE_TRACE() { visitor->Trace(page_); }

 private:
  void BeginResizeScope() { resize_scope_count_++; }
  void EndResizeScope();
  void BeginClampingScope();
  void EndClampingScope();
  LocalFrameView* RootFrameView();

  // The amount of resize-induced clamping drift accumulated during the
  // ResizeScope.  Note that this should NOT include other kinds of scrolling
  // that may occur during layout, such as from ScrollAnchor.
  ScrollOffset drift_;

  // RootFrameViewport scroll offset at start of outer ClampingScope.
  ScrollOffset clamp_start_offset_;

  Member<Page> page_;
  int resize_scope_count_;
  int clamping_scope_count_;
};

}  // namespace blink

#endif  // ResizeViewportAnchor_h
