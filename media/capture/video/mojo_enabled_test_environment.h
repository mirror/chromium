// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAPTURE_VIDEO_MOJO_ENABLED_TEST_ENVIRONMENT
#define MEDIA_CAPTURE_VIDEO_MOJO_ENABLED_TEST_ENVIRONMENT

#include "base/macros.h"
#include "base/threading/thread.h"
#include "mojo/edk/embedder/scoped_ipc_support.h"
#include "testing/gtest/include/gtest/gtest.h"

class MojoEnabledTestEnvironment final : public testing::Environment {
 public:
  MojoEnabledTestEnvironment();
  ~MojoEnabledTestEnvironment();

  // testing::Environment implementation:
  void SetUp() final;
  void TearDown() final;

 private:
  base::Thread mojo_ipc_thread_;
  std::unique_ptr<mojo::edk::ScopedIPCSupport> mojo_ipc_support_;

  DISALLOW_COPY_AND_ASSIGN(MojoEnabledTestEnvironment);
};

#endif  // MEDIA_CAPTURE_VIDEO_MOJO_ENABLED_TEST_ENVIRONMENT
