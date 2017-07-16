// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_TEST_PALETTE_TRAY_TEST_API_H_
#define ASH_TEST_PALETTE_TRAY_TEST_API_H_

#include "ash/system/palette/palette_tool_manager.h"
#include "ash/system/palette/palette_tray.h"
#include "ash/system/tray/tray_bubble_wrapper.h"
#include "base/macros.h"

namespace ash {

class PaletteTray;

namespace test {

class PaletteTrayTestApi {
 public:
  explicit PaletteTrayTestApi(PaletteTray* palette_tray);
  ~PaletteTrayTestApi();

  PaletteToolManager* GetPaletteToolManager() {
    return palette_tray_->palette_tool_manager_.get();
  }

  TrayBubbleWrapper* GetTrayBubbleWrapper() {
    return palette_tray_->bubble_.get();
  }

 private:
  // Not owned.
  PaletteTray* palette_tray_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(PaletteTrayTestApi);
};

}  // namespace test
}  // namespace ash

#endif  // ASH_TEST_PALETTE_TRAY_TEST_API_H_
