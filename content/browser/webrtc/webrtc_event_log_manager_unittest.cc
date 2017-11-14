// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webrtc/webrtc_event_log_manager.h"

#include "base/memory/ptr_util.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::test::ScopedTaskEnvironment;

namespace content {

// TODO(eladalon): !!! After the initial code-review iterations, but before
// landing this CL, add unit-tests.

class WebRtcEventLogManagerTest : public ::testing::Test {
 protected:
  content::TestBrowserThreadBundle test_browser_thread_bundle_;
};

// This class is expected to be created, then left around until it's leaked,
// but it's still good to have its destructor behave sanely.
TEST_F(WebRtcEventLogManagerTest, CreationAndDestructionSanity) {
  auto manager = base::MakeUnique<WebRtcEventLogManager>();
  manager.reset();
}

}  // namespace content
