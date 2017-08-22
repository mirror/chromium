// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gl/init/gl_initializer.h"

#include "base/logging.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_gl_api_implementation.h"
#include "ui/gl/gl_surface.h"
#include "ui/gl/init/ozone_util.h"

namespace gl {
namespace init {

bool InitializeGLOneOffPlatform() {
  LOG(ERROR) << "here5";
  if (HasGLOzone()) {
    LOG(ERROR) << "here6";
    return GetGLOzone()->InitializeGLOneOffPlatform();
  }

  LOG(ERROR) << "here7";
  switch (GetGLImplementation()) {
    LOG(ERROR) << "here8";
    case kGLImplementationMockGL:
    case kGLImplementationStubGL:
      return true;
    default:
      LOG(ERROR) << "here9";
      NOTREACHED();
  }
  LOG(ERROR) << "here10";
  return false;
}

bool InitializeStaticGLBindings(GLImplementation implementation) {
  // Prevent reinitialization with a different implementation. Once the gpu
  // unit tests have initialized with kGLImplementationMock, we don't want to
  // later switch to another GL implementation.
  DCHECK_EQ(kGLImplementationNone, GetGLImplementation());

  LOG(ERROR) << "here0";
  if (HasGLOzone(implementation)) {
    LOG(ERROR) << "here1";
    return GetGLOzone(implementation)
        ->InitializeStaticGLBindings(implementation);
  }

  LOG(ERROR) << "here2";
  switch (implementation) {
    case kGLImplementationMockGL:
    case kGLImplementationStubGL:
      LOG(ERROR) << "here3";
      SetGLImplementation(implementation);
      InitializeStaticGLBindingsGL();
      return true;
    default:
      LOG(ERROR) << "here4";
      NOTREACHED();
  }

  LOG(ERROR) << "here11";
  return false;
}

void InitializeDebugGLBindings() {
  if (HasGLOzone()) {
    GetGLOzone()->InitializeDebugGLBindings();
    return;
  }

  InitializeDebugGLBindingsGL();
}

void ShutdownGLPlatform() {
  if (HasGLOzone()) {
    GetGLOzone()->ShutdownGL();
    return;
  }

  ClearBindingsGL();
}

}  // namespace init
}  // namespace gl
