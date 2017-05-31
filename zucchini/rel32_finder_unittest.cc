// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "zucchini/rel32_finder.h"

#include <cstddef>
#include <sstream>
#include <string>
#include <vector>

#include "base/logging.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "zucchini/image_utils.h"
#include "zucchini/region.h"

namespace zucchini {

namespace {

// A mock Rel32Finder that stores parameters of the last SearchSegment() call.
class TestRel32Finder : public Rel32Finder {
 public:
  TestRel32Finder(Region::const_iterator base,
                  const std::vector<offset_t>& abs32_locations,
                  std::ptrdiff_t abs32_width)
      : Rel32Finder(base, abs32_locations, abs32_width) {}

  // TestRel32Finder interface.
  void SearchSegment(Region::const_iterator seg_begin,
                     Region::const_iterator seg_end,
                     SearchCallback* callback) override {
    seg_begin_cache = seg_begin;
    seg_end_cache = seg_end;
    (*callback)();
  }

  // Parameters of the most recent call to SearchInterval().
  Region::const_iterator seg_begin_cache;
  Region::const_iterator seg_end_cache;
};

}  // namespace

TEST(Rel32FinderTest, SearchSegmentation) {
  const size_t kRegionTotal = 99;
  std::vector<uint8_t> region_data(kRegionTotal);
  Region region(&region_data[0], region_data.size());

  // Common test code that returns the resulting segments as a string.
  auto run_test = [&](size_t rlo, size_t rhi,
                      std::vector<offset_t> abs32_locations,
                      std::ptrdiff_t abs32_width) -> std::string {
    CHECK_LE(rlo, kRegionTotal);
    CHECK_LE(rhi, kRegionTotal);
    CHECK_GT(abs32_width, 0);
    CHECK(std::is_sorted(abs32_locations.begin(), abs32_locations.end()));
    std::ostringstream oss;
    TestRel32Finder finder(region.begin(), abs32_locations, abs32_width);
    // Callback: Grab stored params and render segment as string.
    Rel32Finder::SearchCallback callback = [&]() -> bool {
      std::ptrdiff_t lo = finder.seg_begin_cache - region.begin();
      std::ptrdiff_t hi = finder.seg_end_cache - region.begin();
      oss << "[" << lo << "," << hi << ")";
      return true;
    };
    finder.Search(region.begin() + rlo, region.begin() + rhi, &callback);
    return oss.str();
  };

  // Empty regions yield empty segments.
  EXPECT_EQ("", run_test(0, 0, std::vector<offset_t>(), 4));
  EXPECT_EQ("", run_test(9, 9, std::vector<offset_t>(), 4));
  EXPECT_EQ("", run_test(8, 8, {8}, 4));
  EXPECT_EQ("", run_test(8, 8, {0, 12}, 4));

  // If no abs32 locations exist then the segment is the main range.
  EXPECT_EQ("[0,99)", run_test(0, 99, std::vector<offset_t>(), 4));
  EXPECT_EQ("[20,21)", run_test(20, 21, std::vector<offset_t>(), 4));
  EXPECT_EQ("[51,55)", run_test(51, 55, std::vector<offset_t>(), 4));

  // abs32 locations found near start of main range.
  EXPECT_EQ("[10,20)", run_test(10, 20, {5}, 4));
  EXPECT_EQ("[10,20)", run_test(10, 20, {6}, 4));
  EXPECT_EQ("[11,20)", run_test(10, 20, {7}, 4));
  EXPECT_EQ("[12,20)", run_test(10, 20, {8}, 4));
  EXPECT_EQ("[13,20)", run_test(10, 20, {9}, 4));
  EXPECT_EQ("[14,20)", run_test(10, 20, {10}, 4));
  EXPECT_EQ("[10,11)[15,20)", run_test(10, 20, {11}, 4));

  // abs32 locations found near end of main range.
  EXPECT_EQ("[10,15)[19,20)", run_test(10, 20, {15}, 4));
  EXPECT_EQ("[10,16)", run_test(10, 20, {16}, 4));
  EXPECT_EQ("[10,17)", run_test(10, 20, {17}, 4));
  EXPECT_EQ("[10,18)", run_test(10, 20, {18}, 4));
  EXPECT_EQ("[10,19)", run_test(10, 20, {19}, 4));
  EXPECT_EQ("[10,20)", run_test(10, 20, {20}, 4));
  EXPECT_EQ("[10,20)", run_test(10, 20, {21}, 4));

  // Main range completely eclipsed by abs32 location.
  EXPECT_EQ("", run_test(10, 11, {7}, 4));
  EXPECT_EQ("", run_test(10, 11, {8}, 4));
  EXPECT_EQ("", run_test(10, 11, {9}, 4));
  EXPECT_EQ("", run_test(10, 11, {10}, 4));
  EXPECT_EQ("", run_test(10, 12, {8}, 4));
  EXPECT_EQ("", run_test(10, 12, {9}, 4));
  EXPECT_EQ("", run_test(10, 12, {10}, 4));
  EXPECT_EQ("", run_test(10, 13, {9}, 4));
  EXPECT_EQ("", run_test(10, 13, {10}, 4));
  EXPECT_EQ("", run_test(10, 14, {10}, 4));
  EXPECT_EQ("", run_test(10, 14, {8, 12}, 4));

  // Partial eclipses.
  EXPECT_EQ("[24,25)", run_test(20, 25, {20}, 4));
  EXPECT_EQ("[20,21)", run_test(20, 25, {21}, 4));
  EXPECT_EQ("[20,21)[25,26)", run_test(20, 26, {21}, 4));

  // abs32 location outside main range.
  EXPECT_EQ("[40,60)", run_test(40, 60, {36, 60}, 4));
  EXPECT_EQ("[41,61)", run_test(41, 61, {0, 10, 20, 30, 34, 62, 68, 80}, 4));

  // Change abs32 width.
  EXPECT_EQ("[10,11)[12,14)[16,19)", run_test(10, 20, {9, 11, 14, 15, 19}, 1));
  EXPECT_EQ("", run_test(10, 11, {10}, 1));
  EXPECT_EQ("[18,23)[29,31)", run_test(17, 31, {15, 23, 26, 31}, 3));
  EXPECT_EQ("[17,22)[25,26)[29,30)", run_test(17, 31, {14, 22, 26, 30}, 3));
  EXPECT_EQ("[10,11)[19,20)", run_test(10, 20, {11}, 8));

  // Mixed cases with abs32 width = 4.
  EXPECT_EQ("[10,15)[19,20)[24,25)", run_test(8, 25, {2, 6, 15, 20, 27}, 4));
  EXPECT_EQ("[0,25)[29,45)[49,50)", run_test(0, 50, {25, 45}, 4));
  EXPECT_EQ("[10,20)[28,50)", run_test(10, 50, {20, 24}, 4));
  EXPECT_EQ("[49,50)[54,60)[64,70)[74,80)[84,87)",
            run_test(49, 87, {10, 20, 30, 40, 50, 60, 70, 80, 90}, 4));
  EXPECT_EQ("[0,10)[14,20)[24,25)[29,50)", run_test(0, 50, {10, 20, 25}, 4));
}

}  // namespace zucchini
