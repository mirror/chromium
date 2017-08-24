// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/address_translator.h"

#include <algorithm>
#include <iterator>
#include <sstream>

#include "testing/gtest/include/gtest/gtest.h"

namespace zucchini {

namespace {

// Test case structs. The convention of EXPECT() specifies "expectd" value
// before ""actual". However, AddressTranslator interfaces explicitly state "X
// to Y". So it is clearer in test cases to specify "input" before "expect".
struct OffsetToRVATestCase {
  offset_t input;
  rva_t expect;
};

struct RVAToOffsetTestCase {
  rva_t input;
  offset_t expect;
};

class TestAddressTranslatorBuilder : public AddressTranslatorBuilder {
 public:
  using AddressTranslatorBuilder::AddressTranslatorBuilder;

  // Alternative AddUnit() implementation that parses a visual representation of
  // offset and RVA ranges. Illustrative example:
  // "..AAA...|....aaaa" => "..AAA..." for offsets, and "....aaaa" for RVAs:
  // - "..AAA...": First non-period character is at 2, so |offset_begin| = 2.
  // - "..AAA...": There are 3 non-period characters, so |offset_size| = +3.
  // - "....aaaa": First non-period character is at 4, so |rva_begin| = 4.
  // - "....aaaa": There are 4 non-period characters, so |rva_size| = +4.
  // For the special case of length-0 range, '!' can be used. For example,
  // "...!...." specifies |begin| = 3 and |size| = +0.
  Status AddUnitAsString(const std::string& s) {
    size_t sep = s.find('|');
    CHECK_NE(sep, std::string::npos);
    std::string s1 = s.substr(0, sep);
    std::string s2 = s.substr(sep + 1);

    auto is_blank = [](char ch) { return ch == '.'; };
    auto is_special = [](char ch) { return ch == '.' || ch == '!'; };
    return AddUnit(
        std::find_if_not(s1.begin(), s1.end(), is_blank) - s1.begin(),
        s1.size() - std::count_if(s1.begin(), s1.end(), is_special),
        std::find_if_not(s2.begin(), s2.end(), is_blank) - s2.begin(),
        s2.size() - std::count_if(s2.begin(), s2.end(), is_special));
  }

  const std::vector<Unit>& units() { return units_; }
};

void TestAddUnits(const std::vector<std::string>& specs,
                  AddressTranslatorBuilder::Status expected,
                  const std::string& case_name) {
  TestAddressTranslatorBuilder builder;
  for (const std::string& s : specs) {
    auto add_result = builder.AddUnitAsString(s);
    if (add_result != AddressTranslatorBuilder::kSuccess) {
      EXPECT_EQ(expected, add_result) << case_name;
      return;
    }
  }
  AddressTranslator trans;
  auto build_result = builder.Build(&trans);
  EXPECT_EQ(expected, build_result) << case_name;
}

// A tester for AddressTranslatorBuilder's Unit overlap detection and merging.
class TwoUnitOverlapTester {
 public:
  struct TestCase {
    std::string unit_str;
    AddressTranslatorBuilder::Status expected =
        AddressTranslatorBuilder::kSuccess;
  };

  static void RunTest(const std::string& unit_str1,
                      const std::vector<TestCase>& test_cases) {
    for (size_t i = 0; i < test_cases.size(); ++i) {
      const auto& test_case = test_cases[i];
      const std::string& unit_str2 = test_case.unit_str;
      std::string name =
          (std::ostringstream() << "Case #" << i << ": " << unit_str2).str();
      TestAddUnits({unit_str1, unit_str2}, test_case.expected, name);
      // Switch order. Expect same results.
      TestAddUnits({unit_str2, unit_str1}, test_case.expected, name);
    }
  }
};

}  // namespace

TEST(AddressTranslatorTest, Empty) {
  using ATB = AddressTranslatorBuilder;
  AddressTranslatorBuilder builder;

  AddressTranslator trans;
  EXPECT_EQ(ATB::kSuccess, builder.Build(&trans));
  offset_t fake_offset_begin = trans.fake_offset_begin();

  // Optimized versions.
  CachedOffsetToRVATranslator offset_to_rva(trans);
  CachedRVAToOffsetTranslator rva_to_offset(trans);

  EXPECT_EQ(kInvalidRVA, trans.OffsetToRVA(0U));
  EXPECT_EQ(kInvalidRVA, trans.OffsetToRVA(100U));
  EXPECT_EQ(kInvalidRVA, offset_to_rva.Convert(0U));
  EXPECT_EQ(kInvalidRVA, offset_to_rva.Convert(100U));

  EXPECT_EQ(kInvalidOffset, trans.RVAToOffset(0U));
  EXPECT_EQ(kInvalidOffset, trans.RVAToOffset(100U));
  EXPECT_EQ(kInvalidOffset, rva_to_offset.Convert(0U));
  EXPECT_EQ(kInvalidOffset, rva_to_offset.Convert(100U));

  EXPECT_EQ(kInvalidRVA, trans.OffsetToRVA(fake_offset_begin));
  EXPECT_EQ(kInvalidRVA, offset_to_rva.Convert(fake_offset_begin));
}

TEST(AddressTranslatorTest, Single) {
  using ATB = AddressTranslatorBuilder;
  AddressTranslatorBuilder builder;
  // Offsets to RVA: [10, 30) -> [100, 120).
  EXPECT_EQ(ATB::kSuccess, builder.AddUnit(10U, +20U, 100U, +20U));

  AddressTranslator trans;
  EXPECT_EQ(ATB::kSuccess, builder.Build(&trans));
  offset_t fake_offset_begin = trans.fake_offset_begin();

  // Optimized versions.
  CachedOffsetToRVATranslator offset_to_rva(trans);
  CachedRVAToOffsetTranslator rva_to_offset(trans);
  EXPECT_EQ(30U, fake_offset_begin);  // Test implementation detail.

  // Offsets to RVAs.
  OffsetToRVATestCase test_cases1[] = {
      {0U, kInvalidRVA}, {9U, kInvalidRVA}, {10U, 100U},
      {20U, 110U},       {29U, 119U},       {30U, kInvalidRVA},
  };
  for (auto& test_case : test_cases1) {
    EXPECT_EQ(test_case.expect, trans.OffsetToRVA(test_case.input));
    EXPECT_EQ(test_case.expect, offset_to_rva.Convert(test_case.input));
  }

  // RVAs to offsets.
  RVAToOffsetTestCase test_cases2[] = {
      {0U, kInvalidOffset}, {99U, kInvalidOffset}, {100U, 10U},
      {110U, 20U},          {119U, 29U},           {120U, kInvalidOffset},
  };
  for (auto& test_case : test_cases2) {
    EXPECT_EQ(test_case.expect, trans.RVAToOffset(test_case.input));
    EXPECT_EQ(test_case.expect, rva_to_offset.Convert(test_case.input));
  }
}

TEST(AddressTranslatorTest, SingleDanglingRVA) {
  using ATB = AddressTranslatorBuilder;
  AddressTranslatorBuilder builder;
  // Offsets to RVA: [10, 30) -> [100, 120 + 7), so has dangling RVAs.
  EXPECT_EQ(ATB::kSuccess, builder.AddUnit(10U, +20U, 100U, +20U + 7U));

  AddressTranslator trans;
  EXPECT_EQ(ATB::kSuccess, builder.Build(&trans));
  offset_t fake_offset_begin = trans.fake_offset_begin();
  EXPECT_EQ(30U, fake_offset_begin);  // Test implementation detail.

  // Optimized versions.
  CachedOffsetToRVATranslator offset_to_rva(trans);
  CachedRVAToOffsetTranslator rva_to_offset(trans);

  // Offsets to RVAs.
  OffsetToRVATestCase test_cases1[] = {
      {0U, kInvalidRVA},
      {9U, kInvalidRVA},
      {10U, 100U},
      {20U, 110U},
      {29U, 119U},
      {30U, kInvalidRVA},
      // Fake offsets to dangling RVAs.
      {fake_offset_begin + 100U, kInvalidRVA},
      {fake_offset_begin + 119U, kInvalidRVA},
      {fake_offset_begin + 120U, 120U},
      {fake_offset_begin + 126U, 126U},
      {fake_offset_begin + 127U, kInvalidRVA},
  };
  for (auto& test_case : test_cases1) {
    EXPECT_EQ(test_case.expect, trans.OffsetToRVA(test_case.input));
    EXPECT_EQ(test_case.expect, offset_to_rva.Convert(test_case.input));
  }

  // RVAs to offsets.
  RVAToOffsetTestCase test_cases2[] = {
      {0U, kInvalidOffset},
      {99U, kInvalidOffset},
      {100U, 10U},
      {110U, 20U},
      {119U, 29U},
      // Dangling RVAs to fake offsets.
      {120U, fake_offset_begin + 120U},
      {126U, fake_offset_begin + 126U},
      {127U, kInvalidOffset},
  };
  for (auto& test_case : test_cases2) {
    EXPECT_EQ(test_case.expect, trans.RVAToOffset(test_case.input));
    EXPECT_EQ(test_case.expect, rva_to_offset.Convert(test_case.input));
  }
}

TEST(AddressTranslatorTest, BasicUsage) {
  using ATB = AddressTranslatorBuilder;
  AddressTranslatorBuilder builder;
  // Offsets covered: [10, 30), [40, 70), [70, 110).
  // Map to RVAs: [200, 220 + 5), [300, 330), [100, 140), so has dangling RVAs.
  EXPECT_EQ(ATB::kSuccess, builder.AddUnit(10U, +20U, 200U, +20U + 5U));

  // Extra offset truncated and ignored.
  EXPECT_EQ(ATB::kSuccess, builder.AddUnit(40U, +30U, 300U, +20U));
  // Overlap with previous: Merged.
  EXPECT_EQ(ATB::kSuccess, builder.AddUnit(50U, +20U, 310U, +20U));

  // Tangent with previous but inconsistent; extra offset truncated and ignored.
  EXPECT_EQ(ATB::kSuccess, builder.AddUnit(70U, +40U, 100U, +20U));
  // Tangent with previous and consistent, so merged.
  EXPECT_EQ(ATB::kSuccess, builder.AddUnit(90U, +20U, 120U, +20U));

  AddressTranslator trans;
  EXPECT_EQ(ATB::kSuccess, builder.Build(&trans));
  offset_t fake_offset_begin = trans.fake_offset_begin();
  EXPECT_EQ(110U, fake_offset_begin);  // Test implementation detail.

  // Optimized versions.
  CachedOffsetToRVATranslator offset_to_rva(trans);
  CachedRVAToOffsetTranslator rva_to_offset(trans);

  // Offsets to RVAs.
  OffsetToRVATestCase test_cases1[] = {
      {0U, kInvalidRVA},
      {9U, kInvalidRVA},
      {10U, 200U},
      {20U, 210U},
      {29U, 219U},
      {30U, kInvalidRVA},
      {39U, kInvalidRVA},
      {40U, 300U},
      {55U, 315U},
      {69U, 329U},
      {70U, 100U},
      {90U, 120U},
      {109U, 139U},
      {110U, kInvalidRVA},
      // Fake offsets to dangling RVAs.
      {fake_offset_begin + 220U, 220U},
      {fake_offset_begin + 224U, 224U},
      {fake_offset_begin + 225U, kInvalidRVA},
  };
  for (auto& test_case : test_cases1) {
    EXPECT_EQ(test_case.expect, trans.OffsetToRVA(test_case.input));
    EXPECT_EQ(test_case.expect, offset_to_rva.Convert(test_case.input));
  }

  // RVAs to offsets.
  RVAToOffsetTestCase test_cases2[] = {
      {0U, kInvalidOffset},
      {99U, kInvalidOffset},
      {100U, 70U},
      {120U, 90U},
      {139U, 109U},
      {140U, kInvalidOffset},
      {199U, kInvalidOffset},
      {200U, 10U},
      {210U, 20U},
      {219U, 29U},
      {225U, kInvalidOffset},
      {299U, kInvalidOffset},
      {300U, 40U},
      {315U, 55U},
      {329U, 69U},
      {330U, kInvalidOffset},
      // Dangling RVAs to fake offsets.
      {220U, fake_offset_begin + 220U},
      {224U, fake_offset_begin + 224U},
      {225U, kInvalidOffset},
  };
  for (auto& test_case : test_cases2) {
    EXPECT_EQ(test_case.expect, trans.RVAToOffset(test_case.input));
    EXPECT_EQ(test_case.expect, rva_to_offset.Convert(test_case.input));
  }
}

TEST(AddressTranslatorTest, Overflow) {
  using ATB = AddressTranslatorBuilder;
  // Test assumes that offset_t and rva_t to be 32-bit.
  static_assert(sizeof(offset_t) == 4 && sizeof(rva_t) == 4,
                "Needs to update test.");
  AddressTranslatorBuilder builder1;
  EXPECT_EQ(ATB::kErrorOverflow,
            builder1.AddUnit(0, +0xC0000000U, 0, +0xC0000000U));

  AddressTranslatorBuilder builder2;
  EXPECT_EQ(ATB::kErrorOverflow, builder2.AddUnit(0, +0, 0, +0xC0000000U));

  // Units are okay, but limitations of the heuristic to convert dangling RVA to
  // fake offset, AddressTranslator cannot be built.
  AddressTranslatorBuilder builder3;
  EXPECT_EQ(ATB::kSuccess, builder3.AddUnit(32, +0, 32, +0x50000000U));
  EXPECT_EQ(ATB::kSuccess, builder3.AddUnit(0x50000000U, +16, 0, +16));
  AddressTranslator trans3;
  EXPECT_EQ(ATB::kErrorFakeOffsetBeginTooLarge, builder3.Build(&trans3));
}

// Sanity test for TestAddressTranslatorBuilder::AddUnitAsString();
TEST(AddressTranslatorTest, AddUnitAsString) {
  TestAddressTranslatorBuilder builder1;
  builder1.AddUnitAsString("..A..|.aaa.");
  AddressTranslator::Unit unit1 = builder1.units()[0];
  EXPECT_EQ(2U, unit1.offset_begin);
  EXPECT_EQ(+1U, unit1.offset_size);
  EXPECT_EQ(1U, unit1.rva_begin);
  EXPECT_EQ(+3U, unit1.rva_size);

  TestAddressTranslatorBuilder builder2;
  builder2.AddUnitAsString(".....!...|.bbbbbb...");
  AddressTranslator::Unit unit2 = builder2.units()[0];
  EXPECT_EQ(5U, unit2.offset_begin);
  EXPECT_EQ(+0U, unit2.offset_size);
  EXPECT_EQ(1U, unit2.rva_begin);
  EXPECT_EQ(+6U, unit2.rva_size);
}

// AddressTranslatorBuilder::Build() lists Unit merging examples in comments.
// The format is different from that used by TestAddUnits(), but adapting them
// is easy, so we may as well do so.
TEST(AddressTranslatorTest, OverlapFromComment) {
  using ATB = AddressTranslatorBuilder;
  struct {
    const char* rva_str;  // RVA comes first in this case.
    const char* offset_str;
    ATB::Status expected = ATB::kSuccess;
  } test_cases[] = {
      {"..ssssffff..", "..SSSSFFFF.."},
      {"..ssssffff..", "..SSSS..FFFF.."},
      {"..ssssffff..", "..FFFF..SSSS.."},
      {"..ssssffff..", "..SSOOFF..", ATB::kErrorBadOverlap},
      {"..sssooofff..", "..SSSOOOFFF.."},
      {"..sssooofff..", "..SSSSSOFFFFF..", ATB::kErrorBadOverlap},
      {"..sssooofff..", "..FFOOOOSS..", ATB::kErrorBadOverlap},
      {"..sssooofff..", "..SSSOOOF.."},
      {"..sssooofff..", "..SSSOOOF.."},
      {"..sssooosss..", "..SSSOOOS.."},
      {"..sssooofff..", "..SSSOO.."},
      {"..sssooofff..", "..SSSOFFF..", ATB::kErrorBadOverlapDanglingRVA},
      {"..sssooosss..", "..SSSOOSSSS..", ATB::kErrorBadOverlapDanglingRVA},
      {"..oooooo..", "..OOO.."},
  };

  auto to_period = [](std::string s, char ch) {  // |s| passed by value.
    std::replace(s.begin(), s.end(), ch, '.');
    return s;
  };

  size_t idx = 0;
  for (const auto& test_case : test_cases) {
    std::string base_str =
        std::string(test_case.offset_str) + "|" + test_case.rva_str;
    std::string unit_str1 = to_period(to_period(base_str, 'S'), 's');
    std::string unit_str2 = to_period(to_period(base_str, 'F'), 'f');
    std::string name = (std::ostringstream() << "Case #" << idx).str();
    TestAddUnits({unit_str1, unit_str2}, test_case.expected, name);
    ++idx;
  }
}

TEST(AddressTranslatorTest, Overlap) {
  using ATB = AddressTranslatorBuilder;
  constexpr const char* unit_str1 = "....AAA.......|.....aaa......";

  std::vector<TwoUnitOverlapTester::TestCase> test_cases = {
      //.....AAA......|.....aaa......   The first Unit. NOLINT
      {"....BBB.......|.....bbb......"},
      {"..BBB.........|...bbb........"},
      {"......BBB.....|.......bbb...."},
      {"..BBBBBBBBB...|...bbb........"},  // Extra offset get truncated.
      {"......BBBBBBBB|.......bbb...."},
      {"....BBB.......|.......bbb....", ATB::kErrorBadOverlap},
      {"..BBB.........|.......bbb....", ATB::kErrorBadOverlap},
      {".......BBB....|.......bbb....", ATB::kErrorBadOverlap},
      //....AAA.......|.....aaa......   The first Unit. NOLINT
      {"..........BBB.|.......bbb....", ATB::kErrorBadOverlap},
      {"......BBB.....|.....bbb......", ATB::kErrorBadOverlap},
      {"......BBB.....|..bbb.........", ATB::kErrorBadOverlap},
      {"......BBB.....|bbb...........", ATB::kErrorBadOverlap},
      {"BBB...........|bbb..........."},  // Disjoint.
      {"........BBB...|.........bbb.."},  // Disjoint.
      {"BBB...........|..........bbb."},  // Disjoint, offset elsewhere.
      {".BBB..........|..bbb........."},  // Tangent.
      //....AAA.......|.....aaa......   The first Unit. NOLINT
      {".......BBB....|........bbb..."},  // Tangent.
      {".BBB..........|........bbb..."},  // Tangent, offset elsewhere.
      {"BBBBBB........|bbb..........."},  // Repeat, with extra offsets.
      {"........BBBB..|.........bbb.."},
      {"BBBBBB........|..........bbb."},
      {".BBBBBB.......|..bbb........."},
      {".......BBBBB..|........bbb..."},
      {".BBB..........|........bbb..."},  // Tangent, offset elsewhere.
      //....AAA.......|.....aaa......   The first Unit. NOLINT
      {"..BBB.........|........bbb...", ATB::kErrorBadOverlap},
      {"...BB.........|....bb........"},
      {"....BB........|.....bb......."},
      {".......BB.....|........bb...."},
      {"...BBBBBB.....|....bbbbbb...."},
      {"..BBBBBB......|...bbbbbb....."},
      {"......BBBBBB..|.......bbbbbb."},
      {"BBBBBBBBBBBBBB|bbbbbbbbbbbbbb", ATB::kErrorBadOverlap},
      //....AAA.......|.....aaa......   The first Unit. NOLINT
      {"B.............|b............."},
      {"B.............|.............b"},
      {"....B.........|.....b........"},
      {"....B.........|......b.......", ATB::kErrorBadOverlap},
      {"....B.........|......b.......", ATB::kErrorBadOverlap},
      {"....BBB.......|.....bb......."},
      {"....BBBB......|.....bbb......"},
      {".........BBBBB|.b............"},
      //....AAA.......|.....aaa......   The first Unit. NOLINT
      {"....AAA.......|.....!........"},
      {"....!.........|.....!........"},  // Empty units gets deleted early.
      {"....!.........|..........!..."},  // Forgiving!
  };

  TwoUnitOverlapTester::RunTest(unit_str1, test_cases);
}

TEST(AddressTranslatorTest, OverlapDangling) {
  using ATB = AddressTranslatorBuilder;
  // First Unit has dangling offsets at
  constexpr const char* unit_str1 = "....AAA.......|.....aaaaaa...";

  std::vector<TwoUnitOverlapTester::TestCase> test_cases = {
      //....AAA.......|.....aaaaaa...   The first Unit. NOLINT
      {"....BBB.......|.....bbbbbb..."},
      {"....BBB.......|.....bbbbb...."},
      {"....BBB.......|.....bbbb....."},
      {"....BBB.......|.....bbb......"},
      {".....BBB......|......bbb.....", ATB::kErrorBadOverlapDanglingRVA},
      {".....BB.......|......bbb....."},
      {"....BBB.......|.....bbbbbbbb."},
      {"..BBBBB.......|...bbbbbbbb..."},
      //....AAA.......|.....aaaaaa...   The first Unit. NOLINT
      {"......!.......|.bbb..........", ATB::kErrorBadOverlap},
      {"..BBBBB.......|...bbbbb......"},
      {".......BBB....|.bbb.........."},  // Next offset used: Can go elsewhere.
      {".......BBB....|.bbbb........."},  // Can be another dangling RVA.
      {".......!......|.bbbb........."},  // Same with empty.
      {"......!.......|.......!......"},  // Okay, but actually gets deleted.
      {"......!.......|.......b......", ATB::kErrorBadOverlapDanglingRVA},
      {"......B.......|.......b......"},
      //....AAA.......|.....aaaaaa...   The first Unit. NOLINT
      {"......BBBB....|.......bbbb...", ATB::kErrorBadOverlapDanglingRVA},
      {"......BB......|.......bb.....", ATB::kErrorBadOverlapDanglingRVA},
      {"......BB......|bb............", ATB::kErrorBadOverlap},
  };

  TwoUnitOverlapTester::RunTest(unit_str1, test_cases);
}

// Tests implementation since algorithm is tricky.
TEST(AddressTranslatorTest, Merge) {
  using ATB = AddressTranslatorBuilder;
  // Merge a bunch of overlapping Units into one big Unit.
  std::vector<std::string> test_case1 = {
      "AAA.......|.aaa......",  // Comment to prevent wrap by formatter.
      "AA........|.aa.......",  //
      "..AAA.....|...aaa....",  //
      "....A.....|.....a....",  //
      ".....AAA..|......aaa.",  //
      "........A.|.........a",  //
  };
  // Try all 6! permutations.
  std::sort(test_case1.begin(), test_case1.end());
  do {
    TestAddressTranslatorBuilder builder1;
    for (const auto& s : test_case1)
      EXPECT_EQ(ATB::kSuccess, builder1.AddUnitAsString(s));

    AddressTranslator trans1;
    EXPECT_EQ(ATB::kSuccess, builder1.Build(&trans1));
    EXPECT_EQ(9U, trans1.fake_offset_begin());

    AddressTranslator::Unit expected{0U, +9U, 1U, +9U};
    EXPECT_EQ(1U, trans1.units_sorted_by_offset().size());
    EXPECT_EQ(expected, trans1.units_sorted_by_offset()[0]);
    EXPECT_EQ(1U, trans1.units_sorted_by_rva().size());
    EXPECT_EQ(expected, trans1.units_sorted_by_rva()[0]);
  } while (std::next_permutation(test_case1.begin(), test_case1.end()));

  // Merge RVA-adjacent Units into two Units.
  std::vector<std::string> test_case2 = {
      ".....A..|.a......",  // First Unit.
      "......A.|..a.....",  //
      "A.......|...a....",  // Second Unit: RVA-adjacent to first Unit, but
      ".A......|....a...",  // offset would become inconsistent, so a new
      "..A.....|.....a..",  // Unit gets created.
  };
  // Try all 5! permutations.
  std::sort(test_case2.begin(), test_case2.end());
  do {
    TestAddressTranslatorBuilder builder2;
    for (const auto& s : test_case2)
      EXPECT_EQ(ATB::kSuccess, builder2.AddUnitAsString(s));

    AddressTranslator trans2;
    EXPECT_EQ(ATB::kSuccess, builder2.Build(&trans2));
    EXPECT_EQ(7U, trans2.fake_offset_begin());

    AddressTranslator::Unit expected1{0U, +3U, 3U, +3U};
    AddressTranslator::Unit expected2{5U, +2U, 1U, +2U};
    EXPECT_EQ(2U, trans2.units_sorted_by_offset().size());
    EXPECT_EQ(expected1, trans2.units_sorted_by_offset()[0]);
    EXPECT_EQ(expected2, trans2.units_sorted_by_offset()[1]);
    EXPECT_EQ(2U, trans2.units_sorted_by_rva().size());
    EXPECT_EQ(expected2, trans2.units_sorted_by_rva()[0]);
    EXPECT_EQ(expected1, trans2.units_sorted_by_rva()[1]);
  } while (std::next_permutation(test_case2.begin(), test_case2.end()));
}

}  // namespace zucchini
