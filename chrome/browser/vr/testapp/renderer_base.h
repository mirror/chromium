// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_TESTAPP_RENDERER_BASE_H_
#define CHROME_BROWSER_VR_TESTAPP_RENDERER_BASE_H_

#include "chrome/browser/vr/testapp/renderer.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/native_widget_types.h"

namespace ui {

class RendererBase : public Renderer {
 public:
  RendererBase(gfx::AcceleratedWidget widget, const gfx::Size& size);
  ~RendererBase() override;

 protected:
  float NextFraction();

  gfx::AcceleratedWidget widget_;
  gfx::Size size_;

  int iteration_ = 0;
};

}  // namespace ui

#endif  // CHROME_BROWSER_VR_TESTAPP_RENDERER_BASE_H_
