// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/threading/sequence_local_storage_map.h"

#include <memory>
#include <utility>

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
  ~SetOnDestroy() {
    if (bool_ptr)
      *bool_ptr = true;
  }

  SetOnDestroy(SetOnDestroy&& set_on_destroy) {
    this->bool_ptr = set_on_destroy.bool_ptr;
    set_on_destroy.bool_ptr = nullptr;
  }

 private:
  bool* bool_ptr;

  DISALLOW_COPY_AND_ASSIGN(SetOnDestroy);
};

}  // namespace
TEST(SequenceLocalStorageMapTest, Destructor) {
  bool set_on_destruction = false;

  {
    SequenceLocalStorageMap sequence_local_storage_map;

    ScopedSetSequenceLocalStorageMapForCurrentThread
        scoped_sequence_local_storage_map{&sequence_local_storage_map};

    SetOnDestroy set_on_destroy{&set_on_destruction};

    SequenceLocalStorageMap::ValueDestructorPair value_destructor_pair;
    value_destructor_pair.value = new SetOnDestroy(std::move(set_on_destroy));
    value_destructor_pair.destructor = [](void* ptr) {
      std::default_delete<SetOnDestroy>()(static_cast<SetOnDestroy*>(ptr));
    };

    // Set the ValueDestructorPair in slot 1
    sequence_local_storage_map.Set(1, value_destructor_pair);
  }

  EXPECT_TRUE(set_on_destruction);
}

}  // namespace internal
}  // namespace base
