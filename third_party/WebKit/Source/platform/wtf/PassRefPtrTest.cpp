// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/wtf/RefPtr.h"

#include "platform/wtf/RefCounted.h"
#include "platform/wtf/RefPtr.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace WTF {

class RefCountedClass : public RefCounted<RefCountedClass> {};

TEST(RefPtrTest, MoveConstructor) {
  RefPtr<RefCountedClass> old_pass_ref_ptr =
      AdoptRef(new RefCountedClass());
  RefCountedClass* raw_ptr = old_pass_ref_ptr.Get();
  EXPECT_EQ(raw_ptr->RefCount(), 1);
  RefPtr<RefCountedClass> new_pass_ref_ptr(std::move(old_pass_ref_ptr));
  EXPECT_EQ(old_pass_ref_ptr.Get(), nullptr);
  EXPECT_EQ(new_pass_ref_ptr.Get(), raw_ptr);
  EXPECT_EQ(raw_ptr->RefCount(), 1);
}

}  // namespace WTF
