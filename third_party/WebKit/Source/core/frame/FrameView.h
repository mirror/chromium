// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FrameView_h
#define FrameView_h

#include "core/dom/DocumentLifecycle.h"
#include "core/frame/EmbeddedContentView.h"
#include "platform/geometry/FloatSize.h"

namespace blink {

struct IntrinsicSizingInfo
    : public GarbageCollectedFinalized<IntrinsicSizingInfo> {
  IntrinsicSizingInfo() : has_width(true), has_height(true) {}
  IntrinsicSizingInfo(const IntrinsicSizingInfo& other)
      : size(other.size),
        aspect_ratio(other.aspect_ratio),
        has_width(other.has_width),
        has_height(other.has_height) {}

  FloatSize size;
  FloatSize aspect_ratio;
  bool has_width;
  bool has_height;

  void Transpose();

  virtual void Trace(blink::Visitor*) {}
  virtual ~IntrinsicSizingInfo() {}
};

class CORE_EXPORT FrameView : public EmbeddedContentView {
 public:
  virtual ~FrameView() = default;
  virtual void UpdateViewportIntersectionsForSubtree(
      DocumentLifecycle::LifecycleState) = 0;

  virtual bool GetIntrinsicSizingInfo(IntrinsicSizingInfo&) const = 0;
  virtual bool HasIntrinsicSizingInfo() const = 0;

  bool IsFrameView() const override { return true; }
};

DEFINE_TYPE_CASTS(FrameView,
                  EmbeddedContentView,
                  embedded_content_view,
                  embedded_content_view->IsFrameView(),
                  embedded_content_view.IsFrameView());

}  // namespace blink

#endif  // FrameView_h
