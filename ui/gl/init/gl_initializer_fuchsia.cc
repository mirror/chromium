// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gl/init/gl_initializer.h"

namespace gl {
namespace init {

bool InitializeGLOneOffPlatform() {
  return false;
}

bool InitializeStaticGLBindings(GLImplementation implementation) {
  return false;
}

void InitializeDebugGLBindings() {
  NOTIMPLEMENTED();
}

void ShutdownGLPlatform() {
  NOTREACHED();
}

}  // namespace init
}  // namespace gl
