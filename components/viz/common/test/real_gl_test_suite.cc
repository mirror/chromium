// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/test/real_gl_test_suite.h"

#include "base/message_loop/message_loop.h"
#include "base/threading/thread_id_name_manager.h"
#include "cc/test/paths.h"
#include "ui/gl/test/gl_surface_test_support.h"

namespace viz {

RealGLTestSuite::RealGLTestSuite(int argc, char** argv)
    : base::TestSuite(argc, argv) {}

RealGLTestSuite::~RealGLTestSuite() {}

void RealGLTestSuite::Initialize() {
  base::TestSuite::Initialize();
  gl::GLSurfaceTestSupport::InitializeOneOff();
  cc::CCPaths::RegisterPathProvider();

  message_loop_.reset(new base::MessageLoop);

  base::ThreadIdNameManager::GetInstance()->SetName(
      base::PlatformThread::CurrentId(), "Main");

  base::DiscardableMemoryAllocator::SetInstance(&discardable_memory_allocator_);
}

void RealGLTestSuite::Shutdown() {
  message_loop_ = nullptr;

  base::TestSuite::Shutdown();
}

}  // namespace viz
