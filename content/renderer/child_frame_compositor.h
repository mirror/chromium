// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_CHILD_FRAME_COMPOSITOR_H_
#define CONTENT_RENDERER_CHILD_FRAME_COMPOSITOR_H_

namespace blink {
class WebLayer;
}  // namespace blink

namespace content {

class ChildFrameCompositor {
 public:
  virtual blink::WebLayer* GetLayer() = 0;

  virtual void SetLayer(std::unique_ptr<blink::WebLayer> web_layer) = 0;

  virtual SkBitmap* GetSadPageBitmap() = 0;
};

}  // namespace content

#endif  // CONTENT_RENDERER_CHILD_FRAME_COMPOSITOR_H_
