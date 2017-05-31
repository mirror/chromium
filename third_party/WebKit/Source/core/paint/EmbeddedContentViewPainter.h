// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EmbeddedContentViewPainter_h
#define EmbeddedContentViewPainter_h

#include "platform/wtf/Allocator.h"

namespace blink {

struct PaintInfo;
class LayoutPoint;
class LayoutEmbeddedContentView;

class EmbeddedContentViewPainter {
  STACK_ALLOCATED();

 public:
  EmbeddedContentViewPainter(
      const LayoutEmbeddedContentView& layout_embedded_content_view)
      : layout_embedded_content_view_(layout_embedded_content_view) {}

  void Paint(const PaintInfo&, const LayoutPoint&);
  void PaintContents(const PaintInfo&, const LayoutPoint&);

 private:
  bool IsSelected() const;

  const LayoutEmbeddedContentView& layout_embedded_content_view_;
};

}  // namespace blink

#endif  // EmbeddedContentViewPainter_h
