// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/threading/sequence_local_storage_slot.h"

#include <utility>

#include "base/threading/sequence_local_storage.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {

namespace {

class SequenceLocalStorageSlotTest : public testing::Test {
 public:
  SequenceLocalStorageSlotTest()
      : scoped_sequence_local_storage(&sequence_local_storage) {}

 protected:
  internal::SequenceLocalStorage sequence_local_storage;
  internal::ScopedSetSequenceLocalStorageForCurrentThread
      scoped_sequence_local_storage;

 private:
  DISALLOW_COPY_AND_ASSIGN(SequenceLocalStorageSlotTest);
};

}  // namespace

TEST_F(SequenceLocalStorageSlotTest, GetSet) {
  SequenceLocalStorageSlot<int> slot;

  slot.Set(5);

  EXPECT_EQ(slot.Get(), 5);
}

TEST_F(SequenceLocalStorageSlotTest, SetObjectIsIndependent) {
  bool should_be_false = false;

  SequenceLocalStorageSlot<bool> slot;

  slot.Set(should_be_false);

  bool& slot_bool = slot.Get();
  slot_bool = true;

  EXPECT_TRUE(slot.Get());
  EXPECT_NE(should_be_false, slot.Get());
}

TEST_F(SequenceLocalStorageSlotTest, GetSetMultipleSlots) {
  SequenceLocalStorageSlot<int> slot1;
  SequenceLocalStorageSlot<int> slot2;
  SequenceLocalStorageSlot<int> slot3;

  slot1.Set(1);
  slot2.Set(2);
  slot3.Set(3);

  EXPECT_EQ(slot1.Get(), 1);
  EXPECT_EQ(slot2.Get(), 2);
  EXPECT_EQ(slot3.Get(), 3);

  slot3.Set(4);
  slot2.Set(5);
  slot1.Set(6);

  EXPECT_EQ(slot3.Get(), 4);
  EXPECT_EQ(slot2.Get(), 5);
  EXPECT_EQ(slot1.Get(), 6);
}

}  // namespace base
