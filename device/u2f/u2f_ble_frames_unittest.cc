// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/u2f/u2f_ble_frames.h"

#include "testing/gtest/include/gtest/gtest.h"

#include <vector>

namespace {

std::vector<uint8_t> GetSomeData(size_t size) {
  std::vector<uint8_t> data(size);
  for (size_t i = 0; i < size; ++i)
    data[i] = static_cast<uint8_t>((i * i) & 0xFF);
  return data;
}

}  // namespace

namespace device {

TEST(U2fBleFramesTest, TestSplitAndAssemble) {
  for (size_t size : {0,  1,  16, 17, 18, 20, 21, 22, 35,  36,
                      37, 39, 40, 41, 54, 55, 56, 60, 100, 65535}) {
    SCOPED_TRACE(size);

    U2fBleFrame frame(U2fBleFrame::Command::PING, GetSomeData(size));

    U2fBleFrameInitializationFragment initial;
    std::vector<U2fBleFrameContinuationFragment> other;
    frame.ToFragments(20, &initial, &other);

    EXPECT_EQ(frame.command(), initial.command());
    EXPECT_EQ(frame.data().size(), static_cast<size_t>(initial.data_length()));

    U2fBleFrameAssembler assembler(initial);
    for (const auto& fragment : other) {
      ASSERT_TRUE(assembler.AddFragment(fragment));
    }

    EXPECT_TRUE(assembler.IsDone());
    ASSERT_TRUE(assembler.GetFrame());

    auto result_frame = std::move(*assembler.GetFrame());
    EXPECT_EQ(frame.command(), result_frame.command());
    EXPECT_EQ(frame.data(), result_frame.data());
  }
}

}  // namespace device
