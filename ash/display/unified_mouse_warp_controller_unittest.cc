// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/display/unified_mouse_warp_controller.h"

#include "ash/display/display_util.h"
#include "ash/display/mirror_window_controller.h"
#include "ash/display/mouse_cursor_event_filter.h"
#include "ash/host/ash_window_tree_host.h"
#include "ash/shell.h"
#include "ash/test/ash_test_base.h"
#include "ui/aura/env.h"
#include "ui/aura/window_tree_host.h"
#include "ui/display/display.h"
#include "ui/display/display_finder.h"
#include "ui/display/manager/display_manager.h"
#include "ui/display/screen.h"
#include "ui/display/unified_desktop_utils.h"
#include "ui/events/test/event_generator.h"
#include "ui/wm/core/coordinate_conversion.h"

namespace ash {

namespace {

struct WarpPair {
  gfx::Point native_point_at_edge;
  std::string expected_point_after_warp;
};

}  // namespace

class UnifiedMouseWarpControllerTest : public AshTestBase {
 public:
  UnifiedMouseWarpControllerTest() {}
  ~UnifiedMouseWarpControllerTest() override {}

  void SetUp() override {
    AshTestBase::SetUp();
    display_manager()->SetUnifiedDesktopEnabled(true);
  }

 protected:
  bool FindMirrroingDisplayIdContainingNativePoint(
      const gfx::Point& point_in_native,
      int64_t* display_id,
      gfx::Point* point_in_mirroring_host,
      gfx::Point* point_in_unified_host) {
    for (auto display : display_manager()->software_mirroring_display_list()) {
      display::ManagedDisplayInfo info =
          display_manager()->GetDisplayInfo(display.id());
      if (info.bounds_in_native().Contains(point_in_native)) {
        *display_id = info.id();
        *point_in_unified_host = point_in_native;
        const gfx::Point& origin = info.bounds_in_native().origin();
        // Convert to mirroring host.
        point_in_unified_host->Offset(-origin.x(), -origin.y());
        *point_in_mirroring_host = *point_in_unified_host;
        // Convert from mirroring host to unified host.
        AshWindowTreeHost* ash_host =
            Shell::Get()
                ->window_tree_host_manager()
                ->mirror_window_controller()
                ->GetAshWindowTreeHostForDisplayId(info.id());
        ash_host->AsWindowTreeHost()->ConvertPixelsToDIP(point_in_unified_host);
        return true;
      }
    }
    return false;
  }

  bool TestIfMouseWarpsAt(const gfx::Point& point_in_native) {
    static_cast<UnifiedMouseWarpController*>(
        Shell::Get()->mouse_cursor_filter()->mouse_warp_controller_for_test())
        ->update_location_for_test();
    int64_t orig_mirroring_display_id;
    gfx::Point point_in_unified_host;
    gfx::Point point_in_mirroring_host;
    if (!FindMirrroingDisplayIdContainingNativePoint(
            point_in_native, &orig_mirroring_display_id,
            &point_in_mirroring_host, &point_in_unified_host)) {
      return false;
    }
    // The location of the ozone's native event is relative to the host.
    GetEventGenerator().MoveMouseToWithNative(point_in_unified_host,
                                              point_in_mirroring_host);
    aura::Window* root = Shell::GetPrimaryRootWindow();
    gfx::Point new_location_in_unified_host =
        aura::Env::GetInstance()->last_mouse_location();
    // Convert screen to the host.
    root->GetHost()->ConvertDIPToPixels(&new_location_in_unified_host);

    auto iter = display::FindDisplayContainingPoint(
        display_manager()->software_mirroring_display_list(),
        new_location_in_unified_host);
    if (iter == display_manager()->software_mirroring_display_list().end())
      return false;
    return orig_mirroring_display_id != iter->id();
  }

  MouseCursorEventFilter* event_filter() {
    return Shell::Get()->mouse_cursor_filter();
  }

  UnifiedMouseWarpController* mouse_warp_controller() {
    return static_cast<UnifiedMouseWarpController*>(
        event_filter()->mouse_warp_controller_for_test());
  }

  // |expected_edges| should have a row for each display which contains the
  // expected native bounds of the shared edges with that display in the order
  // "top", "left", "right", "bottom".
  // If |matrix| is empty, default unified layout will be used.
  void BoundaryTestBody(
      const std::string& displays_specs,
      const display::UnifiedDesktopLayoutMatrix& matrix,
      const std::vector<std::vector<std::string>>& expected_edges) {
    UpdateDisplay(displays_specs);
    display_manager()->SetUnifiedDesktopMatrix(matrix);

    // Let the UnifiedMouseWarpController compute the bounds by
    // generating a mouse move event.
    GetEventGenerator().MoveMouseTo(gfx::Point(0, 0));
    const display::Displays& mirroring_displays =
        display_manager()->software_mirroring_display_list();

    ASSERT_EQ(expected_edges.size(), mirroring_displays.size());
    int index = 0;
    for (const auto& display : mirroring_displays) {
      const int64_t id = display.id();
      const auto& display_expected_edges = expected_edges[index++];
      const auto& display_actual_edges =
          mouse_warp_controller()->displays_edges_map_.at(id);
      ASSERT_EQ(display_expected_edges.size(), display_actual_edges.size());
      for (size_t i = 0; i < display_expected_edges.size(); ++i) {
        EXPECT_EQ(display_expected_edges[i],
                  display_actual_edges[i]
                      .edge_native_bounds_in_source_display.ToString());
      }
    }
  }

  void WarpTestBody(const std::vector<WarpPair>& warp_pairs) {
    for (const auto& pair : warp_pairs) {
      EXPECT_TRUE(TestIfMouseWarpsAt(pair.native_point_at_edge));
      EXPECT_EQ(pair.expected_point_after_warp,
                aura::Env::GetInstance()->last_mouse_location().ToString());
    }
  }

  void NoWarpTestBody() {
    // Touch the left edge of the first display.
    EXPECT_FALSE(TestIfMouseWarpsAt(gfx::Point(0, 10)));
    // Touch the top edge of the first display.
    EXPECT_FALSE(TestIfMouseWarpsAt(gfx::Point(10, 0)));
    // Touch the bottom edge of the first display.
    EXPECT_FALSE(TestIfMouseWarpsAt(gfx::Point(10, 499)));

    // Touch the right edge of the second display.
    EXPECT_FALSE(TestIfMouseWarpsAt(gfx::Point(1099, 10)));
    // Touch the top edge of the second display.
    EXPECT_FALSE(TestIfMouseWarpsAt(gfx::Point(610, 0)));
    // Touch the bottom edge of the second display.
    EXPECT_FALSE(TestIfMouseWarpsAt(gfx::Point(610, 499)));
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(UnifiedMouseWarpControllerTest);
};

// Verifies if MouseCursorEventFilter's bounds calculation works correctly.
TEST_F(UnifiedMouseWarpControllerTest, BoundaryTest) {
  {
    SCOPED_TRACE("1x1");
    BoundaryTestBody("400x400,0+450-700x400",
                     {},  // Empty matrix (use horizontal layout).
                     {{"399,0 1x400"}, {"0,450 1x400"}});
    BoundaryTestBody("400x400,0+450-700x600",
                     {},  // Empty matrix (use horizontal layout).
                     {{"399,0 1x400"}, {"0,450 1x600"}});
  }
  {
    SCOPED_TRACE("2x1");
    BoundaryTestBody("400x400*2,0+450-700x400",
                     {},  // Empty matrix (use horizontal layout).
                     {{"399,0 1x400"}, {"0,450 1x400"}});
    BoundaryTestBody("400x400*2,0+450-700x600",
                     {},  // Empty matrix (use horizontal layout).
                     {{"399,0 1x400"}, {"0,450 1x600"}});
  }
  {
    SCOPED_TRACE("1x2");
    BoundaryTestBody("400x400,0+450-700x400*2",
                     {},  // Empty matrix (use horizontal layout).
                     {{"399,0 1x400"}, {"0,450 1x400"}});
    BoundaryTestBody("400x400,0+450-700x600*2",
                     {},  // Empty matrix (use horizontal layout).
                     {{"399,0 1x400"}, {"0,450 1x600"}});
  }
  {
    SCOPED_TRACE("2x2");
    BoundaryTestBody("400x400*2,0+450-700x400*2",
                     {},  // Empty matrix (use horizontal layout).
                     {{"399,0 1x400"}, {"0,450 1x400"}});
    BoundaryTestBody("400x400*2,0+450-700x600*2",
                     {},  // Empty matrix (use horizontal layout).
                     {{"399,0 1x400"}, {"0,450 1x600"}});
  }
}

TEST_F(UnifiedMouseWarpControllerTest, BoundaryTestGrid) {
  // Update displays here first so we get the correct display IDs list. The
  // below are the native bounds.
  const std::string display_specs =
      "0+0-500x300,510+0-400x500,920+0-300x600,"
      "0+600-200x300,210+600-700x200,920+600-350x480,"
      "0+1080-300x500,310+1080-600x600,920+1080-400x450";
  UpdateDisplay(display_specs);
  display_manager()->SetUnifiedDesktopEnabled(true);
  display::DisplayIdList list = display_manager()->GetCurrentDisplayIdList();
  ASSERT_EQ(9u, list.size());

  // Test a very general case of a 3 x 3 matrix.
  // 0:[500 x 300] 1:[400 x 500] 2:[300 x 600]
  // 3:[200 x 300] 4:[700 x 200] 5:[350 x 480]
  // 6:[300 x 500] 7:[600 x 600] 8:[400 x 450]
  display::UnifiedDesktopLayoutMatrix matrix;
  matrix.resize(3u);
  matrix[0].emplace_back(list[0]);
  matrix[0].emplace_back(list[1]);
  matrix[0].emplace_back(list[2]);
  matrix[1].emplace_back(list[3]);
  matrix[1].emplace_back(list[4]);
  matrix[1].emplace_back(list[5]);
  matrix[2].emplace_back(list[6]);
  matrix[2].emplace_back(list[7]);
  matrix[2].emplace_back(list[8]);

  const std::vector<std::vector<std::string>> expected_edges = {
      // Display 0 edges.
      {
          "499,0 1x300",    // Right with display 1.
          "0,299 121x1",    // Bottom with display 3.
          "121,299 379x1",  // Bottom with display 4.
      },
      // Display 1 edges.
      {
          "510,0 1x500",    // Left with display 0.
          "909,0 1x500",    // Right with display 2.
          "510,499 399x1",  // Bottom with display 4.
      },
      // Display 2 edges.
      {
          "920,0 1x600",    // Left with display 1.
          "920,599 38x1",   // Bottom with display 4.
          "958,599 260x1",  // Bottom with display 5.
      },
      // Display 3 edges.
      {
          "0,600 200x1",    // Top with display 0.
          "199,600 1x299",  // Right with display 4.
          "0,899 200x1",    // Bottom with display 6.
      },
      // Display 4 edges.
      {
          "210,600 417x1",  // Top with display 0.
          "627,600 264x1",  // Top with display 1.
          "891,600 21x1",   // Top with display 2.
          "210,600 1x199",  // Left with display 3.
          "909,600 1x199",  // Right with display 5.
          "210,799 102x1",  // Bottom with display 6.
          "312,799 395x1",  // Bottom with display 7.
          "707,799 205x1",  // Bottom with display 8.
      },
      // Display 5 edges.
      {
          "920,600 344x1",   // Top with display 2.
          "920,600 1x480",   // Left with display 4.
          "920,1079 347x1",  // Bottom with display 8.
      },
      // Display 6 edges.
      {
          "0,1080 169x1",    // Top with display 3.
          "169,1080 130x1",  // Top with display 4.
          "299,1080 1x500",  // Right with display 7.
      },
      // Display 7 edges.
      {
          "310,1080 600x1",  // Top with display 4.
          "310,1080 1x599",  // Left with display 6.
          "909,1080 1x599",  // Right with display 8.
      },
      // Display 8 edges.
      {
          "920,1080 234x1",   // Top with display 4.
          "1154,1080 165x1",  // Top with display 5.
          "920,1080 1x450",   // Left with display 7.
      },
  };

  BoundaryTestBody(display_specs, matrix, expected_edges);

  ASSERT_EQ(1, display::Screen::GetScreen()->GetNumDisplays());

  const std::vector<WarpPair> warp_pairs = {
      {{499, 10}, "499,8"},      // Display 0 --> 1.
      {{510, 10}, "496,4"},      // Display 1 --> 0.
      {{10, 299}, "8,299"},      // Display 0 --> 3.
      {{10, 600}, "5,297"},      // Display 3 --> 0.
      {{130, 299}, "128,299"},   // Display 0 --> 4.
      {{250, 600}, "156,297"},   // Display 4 --> 0.
      {{299, 1100}, "214,493"},  // Display 6 --> 7.
      {{350, 1080}, "236,478"},  // Display 7 --> 4.
  };
  WarpTestBody(warp_pairs);
}

// Verifies if the mouse pointer correctly moves to another display in
// unified desktop mode.
TEST_F(UnifiedMouseWarpControllerTest, WarpMouse) {
  UpdateDisplay("500x500,600+0-500x500");
  ASSERT_EQ(1, display::Screen::GetScreen()->GetNumDisplays());

  EXPECT_FALSE(TestIfMouseWarpsAt(gfx::Point(10, 10)));
  // Touch the right edge of the first display. Pointer should warp.
  EXPECT_TRUE(TestIfMouseWarpsAt(gfx::Point(499, 10)));
  EXPECT_EQ("501,10",  // by 2px.
            aura::Env::GetInstance()->last_mouse_location().ToString());

  // Touch the left edge of the second display. Pointer should warp.
  EXPECT_TRUE(TestIfMouseWarpsAt(gfx::Point(600, 10)));
  EXPECT_EQ("498,10",  // by 2px.
            aura::Env::GetInstance()->last_mouse_location().ToString());
  {
    SCOPED_TRACE("1x1 NO WARP");
    NoWarpTestBody();
  }

  // With 2X and 1X displays
  UpdateDisplay("500x500*2,600+0-500x500");
  ASSERT_EQ(1, display::Screen::GetScreen()->GetNumDisplays());

  EXPECT_FALSE(TestIfMouseWarpsAt(gfx::Point(10, 10)));
  // Touch the right edge of the first display. Pointer should warp.
  EXPECT_TRUE(TestIfMouseWarpsAt(gfx::Point(499, 10)));
  EXPECT_EQ("250,5",  // moved to 501 by 2px, divided by 2 (dsf).
            aura::Env::GetInstance()->last_mouse_location().ToString());

  // Touch the left edge of the second display. Pointer should warp.
  EXPECT_TRUE(TestIfMouseWarpsAt(gfx::Point(600, 10)));
  EXPECT_EQ("249,5",  // moved to 498 by 2px, divided by 2 (dsf).
            aura::Env::GetInstance()->last_mouse_location().ToString());

  {
    SCOPED_TRACE("2x1 NO WARP");
    NoWarpTestBody();
  }

  // With 1X and 2X displays
  UpdateDisplay("500x500,600+0-500x500*2");
  ASSERT_EQ(1, display::Screen::GetScreen()->GetNumDisplays());

  EXPECT_FALSE(TestIfMouseWarpsAt(gfx::Point(10, 10)));
  // Touch the right edge of the first display. Pointer should warp.
  EXPECT_TRUE(TestIfMouseWarpsAt(gfx::Point(499, 10)));
  EXPECT_EQ("501,10",  // by 2px.
            aura::Env::GetInstance()->last_mouse_location().ToString());

  // Touch the left edge of the second display. Pointer should warp.
  EXPECT_TRUE(TestIfMouseWarpsAt(gfx::Point(600, 10)));
  EXPECT_EQ("498,10",  // by 2px.
            aura::Env::GetInstance()->last_mouse_location().ToString());
  {
    SCOPED_TRACE("1x2 NO WARP");
    NoWarpTestBody();
  }

  // With two 2X displays
  UpdateDisplay("500x500*2,600+0-500x500*2");
  ASSERT_EQ(1, display::Screen::GetScreen()->GetNumDisplays());

  EXPECT_FALSE(TestIfMouseWarpsAt(gfx::Point(10, 10)));
  // Touch the right edge of the first display. Pointer should warp.
  EXPECT_TRUE(TestIfMouseWarpsAt(gfx::Point(499, 10)));
  EXPECT_EQ("250,5",  // by 2px.
            aura::Env::GetInstance()->last_mouse_location().ToString());

  // Touch the left edge of the second display. Pointer should warp.
  EXPECT_TRUE(TestIfMouseWarpsAt(gfx::Point(600, 10)));
  EXPECT_EQ("249,5",  // moved to 498 by 2px, divided by 2 (dsf).
            aura::Env::GetInstance()->last_mouse_location().ToString());
  {
    SCOPED_TRACE("1x2 NO WARP");
    NoWarpTestBody();
  }
}

}  // namespace aura
