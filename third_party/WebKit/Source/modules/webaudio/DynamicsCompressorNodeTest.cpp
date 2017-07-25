// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/testing/DummyPageHolder.h"
#include "modules/webaudio/BaseAudioContext.h"
#include "modules/webaudio/DynamicsCompressorNode.h"
#include "modules/webaudio/OfflineAudioContext.h"
#include "testing/gtest/include/gtest/gtest.h"
#include <memory>

namespace blink {

TEST(DynamicsCompressorNodeTest, ProcessorLifetime) {
  std::unique_ptr<DummyPageHolder> page = DummyPageHolder::Create();
  OfflineAudioContext* context = OfflineAudioContext::Create(
      &page->GetDocument(), 2, 1, 48000, ASSERT_NO_EXCEPTION);
  DynamicsCompressorNode* node =
      context->createDynamicsCompressor(ASSERT_NO_EXCEPTION);
  DynamicsCompressorHandler& handler = node->GetDynamicsCompressorHandler();
  EXPECT_TRUE(handler.dynamics_compressor_);
  BaseAudioContext::AutoLocker locker(context);
  handler.Dispose();
  // dynamics_compressor_ should live after Dispose() because an audio thread
  // is using it.
  EXPECT_TRUE(handler.dynamics_compressor_);
}

}  // namespace blink
