// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/timing/PerformanceEntry.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

class PerformanceEntryTest : public ::testing::Test {
  void SetUp() override {}
};

TEST_F(PerformanceEntryTest, AllValidEntryTypes) {
  const Vector<PerformanceEntryType>& all_valid_entry_types =
      PerformanceEntry::AllValidEntryTypes();

  // Ensure each enum is in all_valid_entry_types.
  size_t type_index = 0;
  for (unsigned type_enum = 1;
       type_enum < static_cast<int>(PerformanceEntry::EntryType::kMaxEntryType);
       type_enum <<= 1) {
    EXPECT_EQ(all_valid_entry_types[type_index], type_enum);
    type_index++;
  }

  // Ensure all_valid_entry_types doesn't have extra enums.
  EXPECT_EQ(type_index, all_valid_entry_types.size());
}
}  // namespace blink
