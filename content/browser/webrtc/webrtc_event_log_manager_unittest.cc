// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webrtc/webrtc_event_log_manager.h"

#include "base/memory/ptr_util.h"
#include "base/test/scoped_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

using base::test::ScopedTaskEnvironment;

// TODO(eladalon): !!!
class RtcEventLogManagerTest : public ::testing::Test {
 public:
  //  RtcEventLogManagerTest()
  //      : scoped_task_environment_(ScopedTaskEnvironment::MainThreadType::UI)
  //      {}
  //
  //  protected:
  //  ScopedTaskEnvironment scoped_task_environment_;
};

// This class is expected to be created, then left around until it's leaked,
// but it's still good to have its destructor behave sanely.
TEST_F(RtcEventLogManagerTest, CreationAndDestructionSanity) {
  ScopedTaskEnvironment scoped_task_environment_(
      ScopedTaskEnvironment::MainThreadType::UI);

  auto manager = base::MakeUnique<RtcEventLogManager>();
  manager.reset();
}

}  // namespace content
