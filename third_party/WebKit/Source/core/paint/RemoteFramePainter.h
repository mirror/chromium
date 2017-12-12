// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef RemoteFramePainter_h
#define RemoteFramePainter_h

#include "third_party/WebKit/Source/platform/heap/Member.h"
#include "third_party/WebKit/Source/platform/wtf/Allocator.h"

namespace blink {

class CullRect;
class GraphicsContext;
class RemoteFrameView;

class RemoteFramePainter {
  STACK_ALLOCATED();

 public:
  explicit RemoteFramePainter(const RemoteFrameView& view)
      : remote_frame_view_(view) {}

  void paint(GraphicsContext&, const CullRect&) const;

 private:
  Member<const RemoteFrameView> remote_frame_view_;
};

}  // namespace blink

#endif  // RemoteFramePainter_h
