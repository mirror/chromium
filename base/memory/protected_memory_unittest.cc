// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/cfi_flags.h"
#include "base/memory/protected_memory.h"
#include "base/memory/protected_memory_cfi.h"
#include "base/synchronization/lock.h"
#include "base/test/gtest_util.h"
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

#if PROTECTED_MEMORY_ENABLED
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

TEST_F(ProtectedMemoryTest, AssertMemoryIsReadOnly) {
  static PROTECTED_MEMORY_SECTION ProtectedMemory<Data> data;
  AssertMemoryIsReadOnly(&data->foo);
  data.SetReadWrite();
  data.SetReadOnly();
  AssertMemoryIsReadOnly(&data->foo);

  ProtectedMemory<Data> writable_data;
  EXPECT_DCHECK_DEATH({ AssertMemoryIsReadOnly(&writable_data->foo); });
}

TEST_F(ProtectedMemoryTest, FailsIfDefinedOutsideOfProtectMemoryRegion) {
  static ProtectedMemory<Data> data;
  EXPECT_DEATH({ data.SetReadWrite(); }, "&data >= ProtectedMemoryStart");
}

TEST_F(ProtectedMemoryTest, ReadOnlyBeforeReadWrite) {
  static PROTECTED_MEMORY_SECTION ProtectedMemory<Data> data;
  EXPECT_DEATH({ data.SetReadOnly(); }, "writers > 0");
}
#endif  // PROTECTED_MEMORY_ENABLED

unsigned int bad_icall(int i) {
  return 4 + i;
}

struct BadIcallStruct {
  BadIcallStruct() = default;
  BadIcallStruct(int (*fp_)(int)) : fp(fp_) {}
  int (*fp)(int);
};

TEST_F(ProtectedMemoryTest, BadMemberCall) {
  static PROTECTED_MEMORY_SECTION ProtectedMemory<BadIcallStruct> pm(
      reinterpret_cast<int (*)(int)>(&bad_icall));

  EXPECT_EQ(unsanitizedCfiCall(pm, &BadIcallStruct::fp)(1), 5);
#if !BUILDFLAG(CFI_ICALL_CHECK)
  EXPECT_EQ(pm->fp(1), 5);
#elif BUILDFLAG(CFI_ENFORCEMENT_TRAP) || BUILDFLAG(CFI_ENFORCEMENT_DIAGNOSTIC)
  EXPECT_DEATH({ pm->fp(1); }, "");
#endif
}

TEST_F(ProtectedMemoryTest, BadFnPtrCall) {
  static PROTECTED_MEMORY_SECTION ProtectedMemory<int (*)(int)> pm(
      reinterpret_cast<int (*)(int)>(&bad_icall));

  EXPECT_EQ(unsanitizedCfiCall(pm)(1), 5);
#if !BUILDFLAG(CFI_ICALL_CHECK)
  EXPECT_EQ((*pm)(1), 5);
#elif BUILDFLAG(CFI_ENFORCEMENT_TRAP) || BUILDFLAG(CFI_ENFORCEMENT_DIAGNOSTIC)
  EXPECT_DEATH({ (*pm)(1); }, "");
#endif
}

}  // namespace base
