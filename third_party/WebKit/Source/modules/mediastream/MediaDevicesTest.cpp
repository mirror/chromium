// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "bindings/core/v8/ExceptionState.h"
#include "bindings/core/v8/V8BindingForTesting.h"
#include "core/testing/NullExecutionContext.h"
#include "modules/mediastream/MediaDevices.h"
#include "modules/mediastream/MediaStreamConstraints.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

TEST(MediaDevicesTest, GetUserMediaCanBeCalled) {
  V8TestingScope scope;
  auto devices = MediaDevices::Create(scope.GetExecutionContext());
  MediaStreamConstraints constraints;
  ScriptPromise promise = devices->getUserMedia(
      scope.GetScriptState(), constraints, scope.GetExceptionState());
  ASSERT_FALSE(promise.IsEmpty());
  // EXPECT_FALSE(promise.isResolved());
}

TEST(MediaDevicesTest, GetUserMediaFailsWhenProviderMissing) {
  V8TestingScope scope;
  auto devices = MediaDevices::Create(scope.GetExecutionContext());
  // Somehow null out the pointer to the device provider
  MediaStreamConstraints constraints;
  ScriptPromise promise = devices->getUserMedia(
      scope.GetScriptState(), constraints, scope.GetExceptionState());
  // EXPECT_TRUE(promise.isRejected());
}

}  // namespace blink
