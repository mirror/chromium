// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/paint/RemoteFramePainter.h"

#include "platform/graphics/GraphicsContext.h"
#include "platform/graphics/paint/CullRect.h"

namespace blink {

void RemoteFramePainter::paint(GraphicsContext& context,
                               const CullRect& rect) const {
  DCHECK(context.Printing());

  // TODO(weili): Implement painting a place holder for a remote frame.
  //              Also call remote_frame_view_->Print() to inform printing
  //              the actual frame.
}

}  // namespace blink
