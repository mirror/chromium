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

SequenceLocalStorageMap::ValueDestructorPair
SetOnDestroyValueDestructorPairFactory(bool* bool_to_set_on_destroy) {
  SequenceLocalStorageMap::ValueDestructorPair value_destructor_pair;

  value_destructor_pair.value = new SetOnDestroy(bool_to_set_on_destroy);

  value_destructor_pair.destructor = [](void* ptr) {
    std::default_delete<SetOnDestroy>()(static_cast<SetOnDestroy*>(ptr));
  };

  return value_destructor_pair;
}

}  // namespace

// Verify that the destructor is called on a value stored in the
// SequenceLocalStorageMap when SequenceLocalStorageMap is destroyed.
TEST(SequenceLocalStorageMapTest, Destructor) {
  bool set_on_destruction = false;

  {
    SequenceLocalStorageMap sequence_local_storage_map;

    ScopedSetSequenceLocalStorageMapForCurrentThread
        scoped_sequence_local_storage_map{&sequence_local_storage_map};

    SequenceLocalStorageMap::ValueDestructorPair value_destructor_pair =
        SetOnDestroyValueDestructorPairFactory(&set_on_destruction);

    // Set the ValueDestructorPair in slot 1
    sequence_local_storage_map.Set(1, value_destructor_pair);
  }

  EXPECT_TRUE(set_on_destruction);
}

// Verify that overwriting a value already in the SequenceLocalStorageMap
// calls value's destructor.
TEST(SequenceLocalStorageMapTest, DestructorCalledOnSetOverwrite) {
  bool set_on_destruction = false;
  bool set_on_destruction2 = false;

  {
    SequenceLocalStorageMap sequence_local_storage_map;

    ScopedSetSequenceLocalStorageMapForCurrentThread
        scoped_sequence_local_storage_map{&sequence_local_storage_map};

    SequenceLocalStorageMap::ValueDestructorPair value_destructor_pair =
        SetOnDestroyValueDestructorPairFactory(&set_on_destruction);

    SequenceLocalStorageMap::ValueDestructorPair value_destructor_pair2 =
        SetOnDestroyValueDestructorPairFactory(&set_on_destruction2);

    // Set the ValueDestructorPair in slot 1
    sequence_local_storage_map.Set(1, value_destructor_pair);

    ASSERT_FALSE(set_on_destruction);

    // Overwrites the old value in slot 1.
    sequence_local_storage_map.Set(1, value_destructor_pair2);

    // Destructor should've been called for the old value in slot 1.
    EXPECT_TRUE(set_on_destruction);
  }
}

}  // namespace internal
}  // namespace base
