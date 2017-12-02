// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "chrome/installer/zucchini/imposed_match_parser.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/logging.h"
#include "base/optional.h"
#include "chrome/installer/zucchini/buffer_view.h"
#include "chrome/installer/zucchini/disassembler.h"
#include "chrome/installer/zucchini/element_detection.h"
#include "chrome/installer/zucchini/image_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace zucchini {

namespace {

// Test version of SingleProgramDetector that uses a lookup table on the
// first byte read from a given |image|.
class TestElementDetector {
 public:
  TestElementDetector() {}

  base::Optional<Element> Run(ConstBufferView image) const {
    DCHECK_GT(image.size(), 0U);
    uint8_t v = *image.begin();
    auto it = lut_.find(v);
    if (it == lut_.end())
      return base::nullopt;
    return Element(image.region(), it->second);
  }

  void Add(uint8_t byte, ExecutableType exe_type) { lut_[byte] = exe_type; }

 private:
  std::map<uint8_t, ExecutableType> lut_;
};

}  // namespace

TEST(ImposedMatchParserTest, ParseImposedMatches) {
  std::vector<uint8_t> old_data;
  std::vector<uint8_t> new_data;
  auto populate = [](const std::string& s, std::vector<uint8_t>* data) {
    for (char ch : s)
      data->push_back(static_cast<uint8_t>(ch));
  };
  // Pos:             11111111
  //        012345678901234567
  populate("1WW222EEEE", &old_data);
  populate("33eee2222222wwww44", &new_data);

  ConstBufferView old_image(&old_data[0], old_data.size());
  ConstBufferView new_image(&new_data[0], new_data.size());

  TestElementDetector detector;
  detector.Add('W', kExeTypeWin32X86);
  detector.Add('w', kExeTypeWin32X86);
  detector.Add('E', kExeTypeElfX86);
  detector.Add('e', kExeTypeElfX86);
  // Reusable output values.
  std::string prev_imposed_matches;
  size_t num_identical;
  std::vector<ElementMatch> matches;
  std::string message;

  auto run_test = [&](const std::string& imposed_matches) -> bool {
    num_identical = 0;
    matches.clear();
    message.clear();
    prev_imposed_matches = imposed_matches;
    std::ostringstream oss;
    bool ret = ParseImposedMatches(
        imposed_matches, old_image, new_image,
        base::Bind(&TestElementDetector::Run, base::Unretained(&detector)), oss,
        &num_identical, &matches);
    message = oss.str();
    return ret;
  };

  auto run_check = [&](const ElementMatch& match, ExecutableType exe_type,
                       offset_t old_offset, size_t old_size,
                       offset_t new_offset, size_t new_size) {
    EXPECT_EQ(exe_type, match.old_element.exe_type) << prev_imposed_matches;
    EXPECT_EQ(old_offset, match.old_element.offset) << prev_imposed_matches;
    EXPECT_EQ(old_size, match.old_element.size) << prev_imposed_matches;
    EXPECT_EQ(exe_type, match.new_element.exe_type) << prev_imposed_matches;
    EXPECT_EQ(new_offset, match.new_element.offset) << prev_imposed_matches;
    EXPECT_EQ(new_size, match.new_element.size) << prev_imposed_matches;
  };

  // Empty string: Vacuous but valid.
  EXPECT_TRUE(run_test(""));
  EXPECT_EQ(0U, num_identical);
  EXPECT_EQ(0U, matches.size());

  // Full matches. Different permutations give same result.
  for (const std::string& imposed_matches :
       {"1+2=12+4,4+2=5+2,6+4=2+3", "1+2=12+4,6+4=2+3,4+2=5+2",
        "4+2=5+2,1+2=12+4,6+4=2+3", "4+2=5+2,6+4=2+3,1+2=12+4",
        "6+4=2+3,1+2=12+4,4+2=5+2", "6+4=2+3,1+2=12+4,4+2=5+2"}) {
    EXPECT_TRUE(run_test(imposed_matches));
    EXPECT_EQ(1U, num_identical);  // "4+2=5+2"
    EXPECT_EQ(2U, matches.size());
    // Results are sorted by "new" offsets.
    run_check(matches[0], kExeTypeElfX86, 6, 4, 2, 3);
    run_check(matches[1], kExeTypeWin32X86, 1, 2, 12, 4);
  }

  // Single subregion match.
  EXPECT_TRUE(run_test("1+2=12+4"));
  EXPECT_EQ(0U, num_identical);
  EXPECT_EQ(1U, matches.size());
  run_check(matches[0], kExeTypeWin32X86, 1, 2, 12, 4);

  // Single subregion match. We're lax with redundant 0.
  EXPECT_TRUE(run_test("6+04=02+10"));
  EXPECT_EQ(0U, num_identical);
  EXPECT_EQ(1U, matches.size());
  run_check(matches[0], kExeTypeElfX86, 6, 4, 2, 10);

  // Successive elements, no overlap.
  EXPECT_TRUE(run_test("1+1=12+1,2+1=13+1"));
  EXPECT_EQ(0U, num_identical);
  EXPECT_EQ(2U, matches.size());
  run_check(matches[0], kExeTypeWin32X86, 1, 1, 12, 1);
  run_check(matches[1], kExeTypeWin32X86, 2, 1, 13, 1);

  // Overlap in "old" file is okay.
  EXPECT_TRUE(run_test("1+2=12+2,1+2=14+2"));
  EXPECT_EQ(0U, num_identical);
  EXPECT_EQ(2U, matches.size());
  run_check(matches[0], kExeTypeWin32X86, 1, 2, 12, 2);
  run_check(matches[1], kExeTypeWin32X86, 1, 2, 14, 2);

  // Entire files: Have unknown type, so are recognized as such, and ignored.
  EXPECT_TRUE(run_test("0+10=0+18"));
  EXPECT_EQ(0U, num_identical);
  EXPECT_EQ(0U, matches.size());
  EXPECT_EQ("Warning: Skipping unknown type in match: 0+10=0+18.\n", message);

  // Forgive matches that mix known type with unknown type.
  EXPECT_TRUE(run_test("1+2=0+18"));
  EXPECT_EQ(0U, num_identical);
  EXPECT_EQ(0U, matches.size());
  EXPECT_EQ("Warning: Skipping unknown type in match: 1+2=0+18.\n", message);

  EXPECT_TRUE(run_test("0+10=12+4"));
  EXPECT_EQ(0U, num_identical);
  EXPECT_EQ(0U, matches.size());
  EXPECT_EQ("Warning: Skipping unknown type in match: 0+10=12+4.\n", message);

  // Test parse errors, including uint32_t overflow.
  for (const std::string& imposed_matches :
       {"x1+2=12+4,4+2=5+2,6+4=2+3", "x1+2=12+4,4+2=5+2,6+4=2+3x", ",", " ",
        "+2=12+4", "1+2+12+4", "1=2+12+4", " 1+2=12+4", "1+2= 12+4", "1", "1+2",
        "1+2=", "1+2=12", "1+2=12+", "4294967296+2=12+4"}) {
    EXPECT_FALSE(run_test(imposed_matches));
    EXPECT_EQ("Parse error in imposed matches.\n", message);
  }

  for (const std::string& imposed_matches :
       {"1+2=12+4,4+2=5+2x", "1+2=12+4 4+2=5+2", "1+2=12+4,4+2=5+2 ",
        "1+2=12+4 "}) {
    EXPECT_FALSE(run_test(imposed_matches));
    EXPECT_EQ("Imposed matches have invalid delimiter.\n", message);
  }

  // Test bound errors, include 0-size.
  for (const std::string& imposed_matches :
       {"1+10=12+4", "1+2=12+7", "0+11=0+18", "0+12=0+17", "10+1=0+18",
        "0+10=18+1", "0+0=0+18", "0+10=0+0", "1000000000+1=0+1000000000"}) {
    EXPECT_FALSE(run_test(imposed_matches));
    EXPECT_EQ("Imposed matches have out-of-bound entry.\n", message);
  }

  // Test overlap errors. Matches that get ignored are still tested.
  for (const std::string& imposed_matches :
       {"1+2=12+4,4+2=5+2,6+4=2+4", "0+10=0+18,1+2=12+4", "6+4=2+10,3+2=5+2"}) {
    EXPECT_FALSE(run_test(imposed_matches));
    EXPECT_EQ("Imposed matches have overlap in \"new\" file.\n", message);
  }

  // Test type inconsistency.
  EXPECT_FALSE(run_test("1+2=2+3"));
  EXPECT_EQ("Inconsistent types in match: 1+2=2+3.\n", message);

  EXPECT_FALSE(run_test("6+4=12+4"));
  EXPECT_EQ("Inconsistent types in match: 6+4=12+4.\n", message);
}

TEST(ImposedMatchParserTest, FormatElementMatch) {
  constexpr ExecutableType kAnyType = kExeTypeWin32X86;
  EXPECT_EQ("1+2=3+4", FormatElementMatch(ElementMatch{{{1, 2}, kAnyType},
                                                       {{3, 4}, kAnyType}}));
  EXPECT_EQ("1000000000+1=0+1000000000",
            FormatElementMatch(ElementMatch{{{1000000000, 1}, kAnyType},
                                            {{0, 1000000000}, kAnyType}}));
}

}  // namespace zucchini
