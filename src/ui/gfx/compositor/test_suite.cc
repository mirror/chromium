// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/compositor/test_suite.h"

#include "base/message_loop.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebKit.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/ui_base_paths.h"
#include "ui/gfx/compositor/compositor_test_support.h"
#include "ui/gfx/gfx_paths.h"
#include "ui/gfx/gl/gl_implementation.h"

#if defined(USE_WEBKIT_COMPOSITOR)
#include "ui/gfx/compositor/compositor_cc.h"
#endif

CompositorTestSuite::CompositorTestSuite(int argc, char** argv)
    : TestSuite(argc, argv) {}

CompositorTestSuite::~CompositorTestSuite() {}

void CompositorTestSuite::Initialize() {
#if defined(OS_LINUX)
  gfx::InitializeGLBindings(gfx::kGLImplementationOSMesaGL);
#endif
  base::TestSuite::Initialize();

  gfx::RegisterPathProvider();
  ui::RegisterPathProvider();

  // Force unittests to run using en-US so if we test against string
  // output, it'll pass regardless of the system language.
  ui::ResourceBundle::InitSharedInstance("en-US");

  message_loop_.reset(new MessageLoop(MessageLoop::TYPE_UI));
  ui::CompositorTestSupport::Initialize();
}

void CompositorTestSuite::Shutdown() {
  ui::CompositorTestSupport::Terminate();
  message_loop_.reset();

  ui::ResourceBundle::CleanupSharedInstance();

  base::TestSuite::Shutdown();
}
