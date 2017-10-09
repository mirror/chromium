// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/protected_memory.h"
#include "base/synchronization/lock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {

class ProtectedMemoryTest : public ::testing::Test {
 protected:
  // Run tests one at a time. Some of the negative tests can not be made thread
  // safe.
  void SetUp() final { lock.Acquire(); }
  void TearDown() final { lock.Release(); }

  Lock lock;
};

struct Data {
  Data() = default;
  Data(int foo_) : foo(foo_) {}
  int foo;
};

TEST_F(ProtectedMemoryTest, Constructor) {
  static PROTECTED_MEMORY_SECTION ProtectedMemory<Data> data(4);
  EXPECT_EQ(data->foo, 4);
}

TEST_F(ProtectedMemoryTest, Basic) {
  static PROTECTED_MEMORY_SECTION ProtectedMemory<Data> data1, data2;

  data1.SetReadWrite();
  data1->foo = 5;
  EXPECT_EQ(data1->foo, 5);

  data2.SetReadWrite();
  data1.SetReadOnly();
  data2->foo = 15;
  EXPECT_EQ(data2->foo, 15);
  data2.SetReadOnly();
}

#if !defined(PROTECTED_MEMORY_DISABLED)
TEST_F(ProtectedMemoryTest, ReadOnlyOnStart) {
  static PROTECTED_MEMORY_SECTION ProtectedMemory<Data> data;
  EXPECT_DEATH({ data->foo = 6; data.SetReadWrite(); }, "");
}

TEST_F(ProtectedMemoryTest, ReadOnlyAfterCallingReadOnly) {
  static PROTECTED_MEMORY_SECTION ProtectedMemory<Data> data;
  data.SetReadWrite();
  data.SetReadOnly();
  EXPECT_DEATH({ data->foo = 7; }, "");
}

// We are OK corrupting our own memory here
TEST_F(ProtectedMemoryTest, MemoryIsReadOnly) {
  static PROTECTED_MEMORY_SECTION ProtectedMemory<Data> data;
  EXPECT_TRUE(DEBUG_MemoryIsReadOnly(&data->foo));
  data.SetReadWrite();
  EXPECT_FALSE(DEBUG_MemoryIsReadOnly(&data->foo));
  data.SetReadOnly();
  EXPECT_TRUE(DEBUG_MemoryIsReadOnly(&data->foo));
}

TEST_F(ProtectedMemoryTest, FailsIfDefinedOutsideOfProtectMemoryRegion) {
  static ProtectedMemory<Data> data;
  EXPECT_DEATH({ data.SetReadWrite(); }, "&data >= ProtectedMemoryStart");
}

TEST_F(ProtectedMemoryTest, ReadOnlyBeforeReadWrite) {
  static PROTECTED_MEMORY_SECTION ProtectedMemory<Data> data;
  EXPECT_DEATH({ data.SetReadOnly(); }, "writers > 0");
}
#endif  // !defined(PROTECTED_MEMORY_DISABLED)

unsigned int bad_icall() {
  return 5;
}

struct BadIcallStruct {
  BadIcallStruct() = default;
  BadIcallStruct(int (*fp_)()) : fp(fp_) {}
  int (*fp)();
};

TEST_F(ProtectedMemoryTest, ProtectedMemoryBadICall) {
  static PROTECTED_MEMORY_SECTION ProtectedMemory<BadIcallStruct> foo(
      reinterpret_cast<int (*)()>(&bad_icall));

  EXPECT_EQ(ProtectedMemoryMemberCall(foo, fp), 5);
#if defined(CFI_ICALL_ENABLED)
  EXPECT_DEATH({ foo->fp(); }, "");
#else
  EXPECT_EQ(foo->fp(), 5);
#endif  // defined(CFI_ICALL_ENABLED)
}

}  // namespace base
