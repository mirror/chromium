// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file defines tests that implementations of GLImage should pass in order
// to be conformant.

#include "ui/gl/test/gl_image_test_template.h"

namespace gl {

base::Optional<GLImplementation>
GLImageTestDelegateBase::GetPreferedGLImplementation() const {
  return base::nullopt;
}

bool GLImageTestDelegateBase::SkipTest() const {
  return false;
}

}  // namespace gl
