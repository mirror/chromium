// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/CompositorMutatorClient.h"

#include <memory>
#include "base/callback.h"
#include "platform/graphics/CompositorMutationsTarget.h"
#include "platform/graphics/CompositorMutator.h"
#include "platform/wtf/PtrUtil.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;

namespace blink {
namespace {

class StubCompositorMutator : public CompositorMutator {
 public:
  StubCompositorMutator() {}

  bool Mutate(double monotonic_time_now) override { return false; }
};

class MockCompositoMutationsTarget : public CompositorMutationsTarget {
};

TEST(CompositorMutatorClient, CallbackForNullMutationsShouldBeNoop) {
  CompositorMutatorClient client(new StubCompositorMutator);

  EXPECT_TRUE(client.TakeMutations().is_null());
}

}  // namespace
}  // namespace blink
