// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/threading/sequence_local_storage.h"

#include "base/threading/sequence_local_storage_slot.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {
namespace internal {

namespace {

class SetOnDestroy {
 public:
  SetOnDestroy(bool* bool_ptr) : bool_ptr{bool_ptr} {
    DCHECK(bool_ptr);
    DCHECK(!(*bool_ptr));
  }
  ~SetOnDestroy() { *bool_ptr = true; }

 private:
  bool* bool_ptr;

  // DISALLOW_COPY_AND_ASSIGN(SetOnDestroy);
};

}  // namespace
TEST(SequenceLocalStorageTest, Destructor) {
  bool set_on_destruction = false;
  {
    SequenceLocalStorage sequence_local_storage;
    ScopedSetSequenceLocalStorageForCurrentThread scoped_sequence_local_storage{
        &sequence_local_storage};

    SequenceLocalStorageSlot<SetOnDestroy> slot;

    SetOnDestroy set_on_destroy{&set_on_destruction};
    slot.Set((set_on_destroy));
  }

  EXPECT_TRUE(set_on_destruction);
}

}  // namespace internal
}  // namespace base
