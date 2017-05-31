// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "zucchini/rel32_utils.h"

#include <cstddef>
#include <vector>

#include "testing/gtest/include/gtest/gtest.h"
#include "zucchini/arm_utils.h"
#include "zucchini/image_utils.h"

namespace zucchini {

namespace {

// A trivial AddressTranslator that applies constant shift, and does not filter.
class TestAddressTranslator : public AddressTranslator {
 public:
  explicit TestAddressTranslator(std::ptrdiff_t offset_to_rva_adjust)
      : offset_to_rva_adjust_(offset_to_rva_adjust) {}

  // AddressTranslator interfaces.
  RVAToOffsetTranslator GetRVAToOffsetTranslator() const override {
    return [this](rva_t rva) mutable -> offset_t {
      return offset_t(rva - offset_to_rva_adjust_);
    };
  }
  OffsetToRVATranslator GetOffsetToRVATranslator() const override {
    return [this](offset_t offset) mutable -> rva_t {
      return rva_t(offset + offset_to_rva_adjust_);
    };
  }

 private:
  const std::ptrdiff_t offset_to_rva_adjust_;
};

class Rel32UtilsTest : public testing::Test {
 public:
  Rel32UtilsTest() {}

  // Checks that |gen| emits and only emits |expected_refs|, in order.
  void CheckGen(ReferenceGenerator gen,
                const std::vector<Reference>& expected_refs) {
    Reference ref;
    for (Reference expected_ref : expected_refs) {
      EXPECT_TRUE(gen(&ref));
      EXPECT_EQ(expected_ref, ref);
    }
    EXPECT_FALSE(gen(&ref));  // Nothing should be left.
  }

  // Copies displacements from |*bytes1| to |*bytes2| and check results against
  // |exp_1_to_2_bytes|. Then repeat from |*bytes2| to |*byte1| and check
  // against |exp_2_to_1_bytes|. Empty expected bytes mean that failure is
  // expected. The copy function is specified by |copier|. For RVA inputs,
  // we iterate over every pair in |rvas|.
  void CheckCopy(std::vector<uint8_t> exp_1_to_2_bytes,
                 std::vector<uint8_t> exp_2_to_1_bytes,
                 std::vector<uint8_t> bytes1,
                 std::vector<uint8_t> bytes2,
                 ARMCopyDisplacementFun copier,
                 std::vector<rva_t> rvas) {
    Region src1_region(&bytes1[0], bytes1.size());
    Region src2_region(&bytes2[0], bytes2.size());
    for (rva_t src_rva : rvas) {
      for (rva_t dst_rva : rvas) {
        {
          std::vector<uint8_t> out_bytes(bytes2);
          Region out_region(&out_bytes[0], out_bytes.size());
          if (exp_1_to_2_bytes.empty()) {
            EXPECT_FALSE(copier(src_rva, src1_region.begin(), dst_rva,
                                out_region.begin()));
          } else {
            EXPECT_TRUE(copier(src_rva, src1_region.begin(), dst_rva,
                               out_region.begin()));
            EXPECT_EQ(exp_1_to_2_bytes, out_bytes);
          }
        }
        // Copy the other way.
        {
          std::vector<uint8_t> out_bytes(bytes1);
          Region out_region(&out_bytes[0], out_bytes.size());
          if (exp_2_to_1_bytes.empty()) {
            EXPECT_FALSE(copier(src_rva, src2_region.begin(), dst_rva,
                                out_region.begin()));
          } else {
            EXPECT_TRUE(copier(src_rva, src2_region.begin(), dst_rva,
                               out_region.begin()));
            EXPECT_EQ(exp_2_to_1_bytes, out_bytes);
          }
        }
      }
    }
  }
};

}  // namespace

TEST_F(Rel32UtilsTest, ARM32CreateFindRel32) {
  constexpr std::ptrdiff_t kTestOffsetToRVAAdjust = 0x30000;
  TestAddressTranslator trans(kTestOffsetToRVAAdjust);

  // A24.
  std::vector<uint8_t> bytes = {
      0xFF, 0xFF, 0xFF, 0xFF,  // 00030000: (Filler)
      0xFF, 0xFF, 0xFF, 0xFF,  // 00030004: (Filler)
      0x00, 0x00, 0x00, 0xEA,  // 00030008: B   00030010 ; A24
      0xFF, 0xFF, 0xFF, 0xFF,  // 0003000C: (Filler)
      0xFF, 0xFF, 0xFF, 0xEB,  // 00030010: BL  00030014 ; A24
      0xFF, 0xFF, 0xFF, 0xFF,  // 00030014: (Filler)
      0xFC, 0xFF, 0xFF, 0xEB,  // 00030018: BL  00030010 ; A24
      0xF8, 0xFF, 0xFF, 0xEA,  // 0003001C: B   00030004 ; A24
  };
  Region region(&bytes[0], bytes.size());
  // Specify rel32 locations directly, instead of parsing.
  std::vector<offset_t> rel32_locations_A24 = {0x0008U, 0x0010U, 0x0018U,
                                               0x001CU};

  // Generate everything.
  ReferenceGenerator gen1 =
      ARMCreateFindRel32<ARM32Rel32Parser::AddrTraits_A24>(
          trans, region.begin(), rel32_locations_A24, 0x0000U, 0x0020U);
  CheckGen(gen1, {{0x0008U, 0x0010U},
                  {0x0010U, 0x0014U},
                  {0x0018U, 0x0010U},
                  {0x001CU, 0x0004U}});

  // Exclude last.
  ReferenceGenerator gen2 =
      ARMCreateFindRel32<ARM32Rel32Parser::AddrTraits_A24>(
          trans, region.begin(), rel32_locations_A24, 0x0000U, 0x001CU);
  CheckGen(gen2, {{0x0008U, 0x0010U}, {0x0010U, 0x0014U}, {0x0018U, 0x0010U}});

  // Only find one.
  ReferenceGenerator gen3 =
      ARMCreateFindRel32<ARM32Rel32Parser::AddrTraits_A24>(
          trans, region.begin(), rel32_locations_A24, 0x000CU, 0x0018U);
  CheckGen(gen3, {{0x0010U, 0x0014U}});
}

TEST_F(Rel32UtilsTest, ARM32CreateReceiveRel32_Easy) {
  constexpr std::ptrdiff_t kTestOffsetToRVAAdjust = 0x00030000U;
  TestAddressTranslator trans(kTestOffsetToRVAAdjust);
  std::vector<uint8_t> bytes = {
      0xFF, 0xFF,              // 00030000: (Filler)
      0x01, 0xDE,              // 00030002: B   00030008 ; T8
      0xFF, 0xFF, 0xFF, 0xFF,  // 00030004: (Filler)
      0x01, 0xE0,              // 00030008: B   0003000E ; T11
      0xFF, 0xFF,              // 0003000A: (Filler)
      0x80, 0xF3, 0x00, 0x80,  // 0003000C: B   00030010 ; T21
  };
  Region region(&bytes[0], bytes.size());

  ReferenceReceptor rcv1 =
      ARMCreateReceiveRel32<ARM32Rel32Parser::AddrTraits_T8>(trans,
                                                             region.begin());
  rcv1({0x0002U, 0x0004U});
  EXPECT_EQ(0xFF, bytes[0x02]);  // 00030002: B   00030004 ; T8
  EXPECT_EQ(0xDE, bytes[0x03]);

  rcv1({0x0002U, 0x000AU});
  EXPECT_EQ(0x02, bytes[0x02]);  // 00030002: B   0003000A ; T8
  EXPECT_EQ(0xDE, bytes[0x03]);

  ReferenceReceptor rcv2 =
      ARMCreateReceiveRel32<ARM32Rel32Parser::AddrTraits_T11>(trans,
                                                              region.begin());
  rcv2({0x0008U, 0x0008U});
  EXPECT_EQ(0xFE, bytes[0x08]);  // 00030008: B   00030008 ; T11
  EXPECT_EQ(0xE7, bytes[0x09]);
  rcv2({0x0008U, 0x0010U});
  EXPECT_EQ(0x02, bytes[0x08]);  // 00030008: B   00030010 ; T11
  EXPECT_EQ(0xE0, bytes[0x09]);

  ReferenceReceptor rcv3 =
      ARMCreateReceiveRel32<ARM32Rel32Parser::AddrTraits_T21>(trans,
                                                              region.begin());
  rcv3({0x000CU, 0x000AU});
  EXPECT_EQ(0xBF, bytes[0x0C]);  // 0003000C: B   0003000A ; T21
  EXPECT_EQ(0xF7, bytes[0x0D]);
  EXPECT_EQ(0xFD, bytes[0x0E]);
  EXPECT_EQ(0xAF, bytes[0x0F]);
  rcv3({0x000CU, 0x0010U});
  EXPECT_EQ(0x80, bytes[0x0C]);  // 0003000C: B   00030010 ; T21
  EXPECT_EQ(0xF3, bytes[0x0D]);
  EXPECT_EQ(0x00, bytes[0x0E]);
  EXPECT_EQ(0x80, bytes[0x0F]);
}

TEST_F(Rel32UtilsTest, ARM32CreateReceiveRel32_Hard) {
  constexpr std::ptrdiff_t kTestOffsetToRVAAdjust = 0xC0030000U;
  TestAddressTranslator trans(kTestOffsetToRVAAdjust);
  std::vector<uint8_t> bytes = {
      0xFF, 0xFF,              // C0030000: (Filler)
      0x00, 0xF0, 0x00, 0xB8,  // C0030002: B   C0030006 ; T24
      0xFF, 0xFF, 0xFF, 0xFF,  // C0030006: (Filler)
      0x00, 0xF0, 0x7A, 0xE8,  // C003000A: BLX C0030100 ; T24
      0xFF, 0xFF,              // C003000E: (Filler)
      0x00, 0xF0, 0x7A, 0xE8,  // C0030010: BLX C0030108 ; T24
  };
  Region region(&bytes[0], bytes.size());

  ReferenceReceptor rcv =
      ARMCreateReceiveRel32<ARM32Rel32Parser::AddrTraits_T24>(trans,
                                                              region.begin());
  rcv({0x0002U, 0x0000U});
  EXPECT_EQ(0xFF, bytes[0x02]);  // C0030002: B   C0030000 ; T24
  EXPECT_EQ(0xF7, bytes[0x03]);
  EXPECT_EQ(0xFD, bytes[0x04]);
  EXPECT_EQ(0xBF, bytes[0x05]);
  rcv({0x0002U, 0x0008U});
  EXPECT_EQ(0x00, bytes[0x02]);  // C0030002: B   C0030008 ; T24
  EXPECT_EQ(0xF0, bytes[0x03]);
  EXPECT_EQ(0x01, bytes[0x04]);
  EXPECT_EQ(0xB8, bytes[0x05]);

  // BLX complication, with location that's not 4-byte aligned.
  rcv({0x000AU, 0x0010U});
  EXPECT_EQ(0x00, bytes[0x0A]);  // C003000A: BLX C0030010 ; T24
  EXPECT_EQ(0xF0, bytes[0x0B]);
  EXPECT_EQ(0x02, bytes[0x0C]);
  EXPECT_EQ(0xE8, bytes[0x0D]);
  rcv({0x000AU, 0x0100U});
  EXPECT_EQ(0x00, bytes[0x0A]);  // C003000A: BLX C0030100 ; T24
  EXPECT_EQ(0xF0, bytes[0x0B]);
  EXPECT_EQ(0x7A, bytes[0x0C]);
  EXPECT_EQ(0xE8, bytes[0x0D]);
  rcv({0x000AU, 0x0000U});
  EXPECT_EQ(0xFF, bytes[0x0A]);  // C003000A: BLX C0030000 ; T24
  EXPECT_EQ(0xF7, bytes[0x0B]);
  EXPECT_EQ(0xFA, bytes[0x0C]);
  EXPECT_EQ(0xEF, bytes[0x0D]);

  // BLX complication, with location that's 4-byte aligned.
  rcv({0x0010U, 0x0010U});
  EXPECT_EQ(0xFF, bytes[0x10]);  // C0030010: BLX C0030010 ; T24
  EXPECT_EQ(0xF7, bytes[0x11]);
  EXPECT_EQ(0xFE, bytes[0x12]);
  EXPECT_EQ(0xEF, bytes[0x13]);
  rcv({0x0010U, 0x0108U});
  EXPECT_EQ(0x00, bytes[0x10]);  // C0030010: BLX C0030108 ; T24
  EXPECT_EQ(0xF0, bytes[0x11]);
  EXPECT_EQ(0x7A, bytes[0x12]);
  EXPECT_EQ(0xE8, bytes[0x13]);
}

// Test BLX encoding A2, which is an ARM instruction that switches to THUMB2,
// and therefore should have 2-byte alignment.
TEST_F(Rel32UtilsTest, ARM32SwitchToThumb2) {
  const std::ptrdiff_t kTestOffsetToRVAAdjust = 0x80030000U;
  TestAddressTranslator trans(kTestOffsetToRVAAdjust);
  std::vector<uint8_t> bytes = {
      0xFF, 0xFF, 0x00, 0x00,  // 80030000: (Filler)
      0x00, 0x00, 0x00, 0xFA,  // 80030004: BLX 8003000C ; A24
  };
  Region region(&bytes[0], bytes.size());

  ReferenceReceptor rcv =
      ARMCreateReceiveRel32<ARM32Rel32Parser::AddrTraits_A24>(trans,
                                                              region.begin());

  // To location that's 4-byte aligned.
  rcv({0x0004U, 0x0100U});
  EXPECT_EQ(0x3D, bytes[0x04]);  // 80030004: BLX 80030100 ; A24
  EXPECT_EQ(0x00, bytes[0x05]);
  EXPECT_EQ(0x00, bytes[0x06]);
  EXPECT_EQ(0xFA, bytes[0x07]);

  // To location that's 2-byte aligned but not 4-byte aligned.
  rcv({0x0004U, 0x0052U});
  EXPECT_EQ(0x11, bytes[0x04]);  // 80030004: BLX 80030052 ; A24
  EXPECT_EQ(0x00, bytes[0x05]);
  EXPECT_EQ(0x00, bytes[0x06]);
  EXPECT_EQ(0xFB, bytes[0x07]);

  // Clean slate code.
  rcv({0x0004U, 0x000CU});
  EXPECT_EQ(0x00, bytes[0x04]);  // 80030004: BLX 8003000C ; A24
  EXPECT_EQ(0x00, bytes[0x05]);
  EXPECT_EQ(0x00, bytes[0x06]);
  EXPECT_EQ(0xFA, bytes[0x07]);
}

TEST_F(Rel32UtilsTest, ARM32CopyDisplacement) {
  std::vector<uint8_t> expect_fail;

  // Successful A24.
  ARMCopyDisplacementFun copier_A24 =
      ARMCopyDisplacement<ARM32Rel32Parser::AddrTraits_A24>;
  CheckCopy({0x12, 0x34, 0x56, 0xEB}, {0xA0, 0xC0, 0x0E, 0x2A},
            {0x12, 0x34, 0x56, 0x2A}, {0xA0, 0xC0, 0x0E, 0xEB}, copier_A24,
            {0, 4, 0x000FFFF8});

  // Successful T8.
  ARMCopyDisplacementFun copier_T8 =
      ARMCopyDisplacement<ARM32Rel32Parser::AddrTraits_T8>;
  CheckCopy({0x12, 0xD5}, {0xAB, 0xD8}, {0x12, 0xD8}, {0xAB, 0xD5}, copier_T8,
            {0, 2, 4, 0x000FFFF8, 0x000FFFFA});

  // Successful T11.
  ARMCopyDisplacementFun copier_T11 =
      ARMCopyDisplacement<ARM32Rel32Parser::AddrTraits_T11>;
  CheckCopy({0xF5, 0xE0}, {0x12, 0xE7}, {0xF5, 0xE0}, {0x12, 0xE7}, copier_T11,
            {0, 2, 4, 0x000FFFF8, 0x000FFFFA});

  // Failure if we use the wrong copier.
  CheckCopy(expect_fail, expect_fail, {0xF5, 0xE0}, {0x12, 0xE7}, copier_T8,
            {0, 2, 4, 0x000FFFF8, 0x000FFFFA});

  // Successful T21.
  ARMCopyDisplacementFun copier_T21 =
      ARMCopyDisplacement<ARM32Rel32Parser::AddrTraits_T21>;
  CheckCopy({0x41, 0xF2, 0xA5, 0x88}, {0x84, 0xF3, 0x3C, 0xA2},
            {0x81, 0xF3, 0xA5, 0x88}, {0x44, 0xF2, 0x3C, 0xA2}, copier_T21,
            {0, 2, 4, 0x000FFFF8, 0x000FFFFA});
  CheckCopy({0x7F, 0xF6, 0xFF, 0xAF}, {0x80, 0xF3, 0x00, 0x80},
            {0xBF, 0xF7, 0xFF, 0xAF}, {0x40, 0xF2, 0x00, 0x80}, copier_T21,
            {0, 2, 4, 0x000FFFF8, 0x000FFFFA});

  // T24: Mix B encoding T4 and BL encoding T1.
  ARMCopyDisplacementFun copier_T24 =
      ARMCopyDisplacement<ARM32Rel32Parser::AddrTraits_T24>;
  CheckCopy({0xFF, 0xF7, 0xFF, 0xFF}, {0x00, 0xF0, 0x00, 0x90},
            {0xFF, 0xF7, 0xFF, 0xBF}, {0x00, 0xF0, 0x00, 0xD0}, copier_T24,
            {0, 2, 4, 0x000FFFF8, 0x000FFFFA});

  // Mix B encoding T4 and BLX encoding T2. Note that the forward direction
  // fails because B's target is invalid for BLX! It's possible to do "best
  // effort" copying to reduce diff -- but right now we're not doing this.
  CheckCopy(expect_fail, {0x00, 0xF0, 0x00, 0x90}, {0xFF, 0xF7, 0xFF, 0xBF},
            {0x00, 0xF0, 0x00, 0xC0}, copier_T24,
            {0, 2, 4, 0x000FFFF8, 0x000FFFFA});
  // Now B's target is valid for BLX.
  CheckCopy({0xFF, 0xF7, 0xFE, 0xEF}, {0x00, 0xF0, 0x00, 0x90},
            {0xFF, 0xF7, 0xFE, 0xBF}, {0x00, 0xF0, 0x00, 0xC0}, copier_T24,
            {0, 2, 4, 0x000FFFF8, 0x000FFFFA});
}

TEST_F(Rel32UtilsTest, AArch64CreateFindRel32) {
  constexpr std::ptrdiff_t kTestOffsetToRVAAdjust = 0x30000;
  TestAddressTranslator trans(kTestOffsetToRVAAdjust);

  std::vector<uint8_t> bytes = {
      0xFF, 0xFF, 0xFF, 0xFF,  // 00030000: (Filler)
      0xFF, 0xFF, 0xFF, 0xFF,  // 00030004: (Filler)
      0x02, 0x00, 0x00, 0x14,  // 00030008: B    00030010 ; Immd26
      0xFF, 0xFF, 0xFF, 0xFF,  // 0003000C: (Filler)
      0x25, 0x00, 0x00, 0x35,  // 00030010: CBNZ R5,00030014 ; Immd19
      0xFF, 0xFF, 0xFF, 0xFF,  // 00030014: (Filler)
      0xCA, 0xFF, 0xFF, 0x54,  // 00030018: BGE  00030010 ; Immd19
      0x4C, 0xFF, 0x8F, 0x36,  // 0003001C: TBZ  X12,#17,00030004 ; Immd14
  };
  Region region(&bytes[0], bytes.size());

  // Generate Immd26. We specify rel32 locations directly.
  std::vector<offset_t> rel32_locations_Immd26 = {0x0008U};
  ReferenceGenerator gen1 =
      ARMCreateFindRel32<AArch64Rel32Parser::AddrTraits_Immd26>(
          trans, region.begin(), rel32_locations_Immd26, 0x0000U, 0x0020U);
  CheckGen(gen1, {{0x0008U, 0x0010U}});

  // Generate Immd19.
  std::vector<offset_t> rel32_locations_Immd19 = {0x0010U, 0x0018U};
  ReferenceGenerator gen2 =
      ARMCreateFindRel32<AArch64Rel32Parser::AddrTraits_Immd19>(
          trans, region.begin(), rel32_locations_Immd19, 0x0000U, 0x0020U);
  CheckGen(gen2, {{0x0010U, 0x0014U}, {0x0018U, 0x0010U}});

  // Generate Immd14.
  std::vector<offset_t> rel32_locations_Immd14 = {0x001CU};
  ReferenceGenerator gen3 =
      ARMCreateFindRel32<AArch64Rel32Parser::AddrTraits_Immd14>(
          trans, region.begin(), rel32_locations_Immd14, 0x0000U, 0x0020U);
  CheckGen(gen3, {{0x001CU, 0x0004U}});
}

TEST_F(Rel32UtilsTest, AArch64CopyDisplacement) {
  std::vector<uint8_t> expect_fail;
  // Use the same set of RVAs: Everything is 4-byte aligned.
  std::vector<rva_t> rvas = {0, 4, 0x000FFFF8};

  // Successful Imm26.
  ARMCopyDisplacementFun copier_Immd26 =
      ARMCopyDisplacement<AArch64Rel32Parser::AddrTraits_Immd26>;
  CheckCopy({0x12, 0x34, 0x56, 0x94}, {0xA1, 0xC0, 0x0E, 0x17},
            {0x12, 0x34, 0x56, 0x14}, {0xA1, 0xC0, 0x0E, 0x97}, copier_Immd26,
            rvas);

  // Successful Imm19.
  ARMCopyDisplacementFun copier_Immd19 =
      ARMCopyDisplacement<AArch64Rel32Parser::AddrTraits_Immd19>;
  CheckCopy({0x24, 0x12, 0x34, 0x54}, {0xD7, 0xA5, 0xFC, 0xB4},
            {0x37, 0x12, 0x34, 0xB4}, {0xC4, 0xA5, 0xFC, 0x54}, copier_Immd19,
            rvas);

  // Successful Imm14.
  ARMCopyDisplacementFun copier_Immd14 =
      ARMCopyDisplacement<AArch64Rel32Parser::AddrTraits_Immd14>;
  CheckCopy({0x00, 0x00, 0x00, 0x36}, {0xFF, 0xFF, 0xFF, 0xB7},
            {0x1F, 0x00, 0xF8, 0xB7}, {0xE0, 0xFF, 0x07, 0x36}, copier_Immd14,
            rvas);

  // Failure if we use incorrect copier.
  CheckCopy(expect_fail, expect_fail, {0x1F, 0x00, 0xF8, 0xB7},
            {0xE0, 0xFF, 0x07, 0x36}, copier_Immd26, rvas);
}

}  // namespace zucchini
