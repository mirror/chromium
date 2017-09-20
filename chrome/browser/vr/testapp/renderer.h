// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_TESTAPP_RENDERER_H_
#define CHROME_BROWSER_VR_TESTAPP_RENDERER_H_

namespace ui {

class Renderer {
 public:
  virtual ~Renderer() {}

  virtual bool Initialize() = 0;
};

}  // namespace ui

#endif  // CHROME_BROWSER_VR_TESTAPP_RENDERER_H_
