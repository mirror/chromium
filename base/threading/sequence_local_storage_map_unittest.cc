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
  SetOnDestroy(bool* bool_ptr) : bool_ptr_{bool_ptr} {
    DCHECK(bool_ptr_);
    DCHECK(!(*bool_ptr_));
  }
  ~SetOnDestroy() {
    if (bool_ptr_)
      *bool_ptr_ = true;
  }

 private:
  bool* bool_ptr_;

  DISALLOW_COPY_AND_ASSIGN(SetOnDestroy);
};

template <typename T, typename... Args>
SequenceLocalStorageMap::ValueDestructorPair CreateValueDestructorPair(
    Args... args) {
  SequenceLocalStorageMap::ValueDestructorPair value_destructor_pair;

  value_destructor_pair.value = new T(args...);

  value_destructor_pair.destructor = [](void* ptr) {
    std::default_delete<T>()(static_cast<T*>(ptr));
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
        CreateValueDestructorPair<SetOnDestroy>(&set_on_destruction);

    // Set the ValueDestructorPair in slot 1
    sequence_local_storage_map.Set(1, value_destructor_pair);
  }

  EXPECT_TRUE(set_on_destruction);
}

// Verify that overwriting a value already in the SequenceLocalStorageMap
// calls value's destructor.
TEST(SequenceLocalStorageMapTest, DestructorCalledOnSetOverwrite) {
  constexpr int SLOT_NUM = 1;

  bool set_on_destruction = false;
  bool set_on_destruction2 = false;
  {
    SequenceLocalStorageMap sequence_local_storage_map;

    ScopedSetSequenceLocalStorageMapForCurrentThread
        scoped_sequence_local_storage_map{&sequence_local_storage_map};

    SequenceLocalStorageMap::ValueDestructorPair value_destructor_pair =
        CreateValueDestructorPair<SetOnDestroy>(&set_on_destruction);

    SequenceLocalStorageMap::ValueDestructorPair value_destructor_pair2 =
        CreateValueDestructorPair<SetOnDestroy>(&set_on_destruction2);

    sequence_local_storage_map.Set(SLOT_NUM, value_destructor_pair);

    ASSERT_FALSE(set_on_destruction);

    // Overwrites the old value in the slot.
    sequence_local_storage_map.Set(SLOT_NUM, value_destructor_pair2);

    // Destructor should've been called for the old value in the slot, and not
    // yet called for the new value.
    EXPECT_TRUE(set_on_destruction);
    EXPECT_FALSE(set_on_destruction2);
  }
  EXPECT_TRUE(set_on_destruction2);
}

TEST(SequenceLocalStorageMapTest, GetSet) {
  constexpr int SLOT_NUM = 1;

  SequenceLocalStorageMap sequence_local_storage_map;
  ScopedSetSequenceLocalStorageMapForCurrentThread
      scoped_sequence_local_storage_map{&sequence_local_storage_map};

  SequenceLocalStorageMap::ValueDestructorPair value_destructor_pair =
      CreateValueDestructorPair<int>(5);

  sequence_local_storage_map.Set(SLOT_NUM, value_destructor_pair);

  EXPECT_EQ(*static_cast<int*>(sequence_local_storage_map.Get(SLOT_NUM)), 5);
}

}  // namespace internal
}  // namespace base
