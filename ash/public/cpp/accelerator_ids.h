// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_PUBLIC_CPP_ACCELERATOR_IDS_H_
#define ASH_PUBLIC_CPP_ACCELERATOR_IDS_H_

#include <stdint.h>

namespace ash {

// Well known ash accelerator ids to share with browser. These ids should be
// negative to avoid overlapping with browser command id.
enum class AcceleratorId : int {
  NEW_INCOGNITO_WINDOW = INT_MIN,
  NEW_TAB,
  NEW_WINDOW,
  OPEN_FEEDBACK_PAGE,
  RESTORE_TAB,
  SHOW_TASK_MANAGER,
};

}  // namespace ash

#endif  // ASH_PUBLIC_CPP_ACCELERATOR_IDS_H_
