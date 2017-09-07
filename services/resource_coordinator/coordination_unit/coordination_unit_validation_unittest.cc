// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/resource_coordinator/coordination_unit/coordination_unit_validation.h"

#include <set>

#include "services/resource_coordinator/coordination_unit/frame_cu.h"
#include "services/resource_coordinator/coordination_unit/page_cu.h"
#include "services/resource_coordinator/coordination_unit/process_cu.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace resource_coordinator {

TEST(CoordinationUnitValidationTest, AppearsToWork) {
  ProcessCU proc1;
  ProcessCU proc2;
  PageCU page1;
  PageCU page2;
  FrameCU frame1_1;
  FrameCU frame1_2;
  FrameCU frame1_3;
  FrameCU frame2_1;
  FrameCU frame2_2;

  std::set<CoordinationUnitBase*> cus{
      &proc1, &proc2, &page1, &page2, &frame1_1, &frame1_2, &frame1_3,
      &frame2_1, &frame2_2};

  EXPECT_FALSE(IsGloballyValid(cus));

  // Make sure everything has a process parent.
  LinkParentChild(&proc1, &page1);
  LinkParentChild(&proc1, &page2);
  LinkParentChild(&proc1, &frame1_1);
  LinkParentChild(&proc1, &frame1_2);
  LinkParentChild(&proc1, &frame2_1);
  LinkParentChild(&proc2, &frame1_3);
  LinkParentChild(&proc2, &frame2_2);

  // Set up the frame tree for page1.
  LinkParentChild(&page1, &frame1_1);
  LinkParentChild(&frame1_1, &frame1_2);
  LinkParentChild(&frame1_1, &frame1_3);

  // Set up the frame tree for page2.
  LinkParentChild(&page2, &frame2_1);
  LinkParentChild(&frame2_1, &frame2_2);

  EXPECT_TRUE(IsGloballyValid(&proc1));
  EXPECT_TRUE(IsGloballyValid(cus));

  // Try the single relationship accessors.
  frame2_2.GetParent<ProcessCU>();
  frame2_2.ClearParent<ProcessCU>();
  frame2_2.SetParent(&proc2);

  EXPECT_TRUE(IsGloballyValid(&proc1));
  EXPECT_TRUE(IsGloballyValid(cus));
}

}  // namespace resource_coordinator
