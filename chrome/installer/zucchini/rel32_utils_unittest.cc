// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/rel32_utils.h"

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <utility>
#include <vector>

#include "base/memory/ptr_util.h"
#include "base/optional.h"
#include "base/test/gtest_util.h"
#include "chrome/installer/zucchini/address_translator.h"
#include "chrome/installer/zucchini/arm_utils.h"
#include "chrome/installer/zucchini/image_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace zucchini {

namespace {

// A trivial AddressTranslator that applies constant shift.
class TestAddressTranslator : public AddressTranslator {
 public:
  TestAddressTranslator(offset_t image_size, rva_t rva_begin) {
    DCHECK_GE(rva_begin, 0U);
    CHECK_EQ(AddressTranslator::kSuccess,
             Initialize({{0, image_size, rva_begin, image_size}}));
  }
};

// Checks that |reader| emits and only emits |expected_refs|, in order.
void CheckReader(const std::vector<Reference>& expected_refs,
                 ReferenceReader* reader) {
  for (Reference expected_ref : expected_refs) {
    auto ref = reader->GetNext();
    EXPECT_TRUE(ref.has_value());
    EXPECT_EQ(expected_ref, ref.value());
  }
  EXPECT_EQ(base::nullopt, reader->GetNext());  // Nothing should be left.
}

}  // namespace

TEST(Rel32UtilsTest, Rel32ReaderX86) {
  constexpr offset_t kTestImageSize = 0x00100000U;
  constexpr rva_t kRvaBegin = 0x00030000U;
  TestAddressTranslator translator(kTestImageSize, kRvaBegin);

  // For simplicity, test data is not real X86 machine code. We are only
  // including rel32 targets, without the full instructions.
  std::vector<uint8_t> bytes = {
      0xFF, 0xFF, 0xFF, 0xFF,  // 00030000: (Filler)
      0x00, 0x00, 0x00, 0x80,  // 00030004: 80030008 Marked, so invalid.
      0x04, 0x00, 0x00, 0x00,  // 00030008: 00030010
      0xFF, 0xFF, 0xFF, 0xFF,  // 0003000C: (Filler)
      0x00, 0x00, 0x00, 0x00,  // 00030010: 00030014
      0xFF, 0xFF, 0xFF, 0xFF,  // 00030014: (Filler)
      0xF4, 0xFF, 0xFF, 0xFF,  // 00030018: 00030010
      0xE4, 0xFF, 0xFF, 0xFF,  // 0003001C: 00030004
  };
  ConstBufferView buffer(bytes.data(), bytes.size());
  // Specify rel32 locations directly, instead of parsing.
  std::vector<offset_t> rel32_locations = {0x0008U, 0x0010U, 0x0018U, 0x001CU};

  // Generate everything.
  Rel32ReaderX86 reader1(buffer, 0x0000U, 0x0020U, &rel32_locations,
                         translator);
  CheckReader({{0x0008U, 0x0010U},
               {0x0010U, 0x0014U},
               {0x0018U, 0x0010U},
               {0x001CU, 0x0004U}},
              &reader1);

  // Exclude last.
  Rel32ReaderX86 reader2(buffer, 0x0000U, 0x001CU, &rel32_locations,
                         translator);
  CheckReader({{0x0008U, 0x0010U}, {0x0010U, 0x0014U}, {0x0018U, 0x0010U}},
              &reader2);

  // Only find one.
  Rel32ReaderX86 reader3(buffer, 0x000CU, 0x0018U, &rel32_locations,
                         translator);
  CheckReader({{0x0010U, 0x0014U}}, &reader3);

  // Marked target encountered (error).
  std::vector<offset_t> rel32_marked_locations = {0x00004U};
  Rel32ReaderX86 reader4(buffer, 0x0000U, 0x0020U, &rel32_marked_locations,
                         translator);
  EXPECT_DCHECK_DEATH(reader4.GetNext());
}

TEST(Rel32UtilsTest, Rel32WriterX86) {
  constexpr offset_t kTestImageSize = 0x00100000U;
  constexpr rva_t kRvaBegin = 0x00030000U;
  TestAddressTranslator translator(kTestImageSize, kRvaBegin);

  std::vector<uint8_t> bytes(32, 0xFF);
  MutableBufferView buffer(bytes.data(), bytes.size());

  Rel32WriterX86 writer(buffer, translator);
  writer.PutNext({0x0008U, 0x0010U});
  EXPECT_EQ(0x00000004U, buffer.read<uint32_t>(0x08));  // 00030008: 00030010

  writer.PutNext({0x0010U, 0x0014U});
  EXPECT_EQ(0x00000000U, buffer.read<uint32_t>(0x10));  // 00030010: 00030014

  writer.PutNext({0x0018U, 0x0010U});
  EXPECT_EQ(0xFFFFFFF4U, buffer.read<uint32_t>(0x18));  // 00030018: 00030010

  writer.PutNext({0x001CU, 0x0004U});
  EXPECT_EQ(0xFFFFFFE4U, buffer.read<uint32_t>(0x1C));  // 0003001C: 00030004

  EXPECT_EQ(std::vector<uint8_t>({
                0xFF, 0xFF, 0xFF, 0xFF,  // 00030000: (Filler)
                0xFF, 0xFF, 0xFF, 0xFF,  // 00030004: (Filler)
                0x04, 0x00, 0x00, 0x00,  // 00030008: 00030010
                0xFF, 0xFF, 0xFF, 0xFF,  // 0003000C: (Filler)
                0x00, 0x00, 0x00, 0x00,  // 00030010: 00030014
                0xFF, 0xFF, 0xFF, 0xFF,  // 00030014: (Filler)
                0xF4, 0xFF, 0xFF, 0xFF,  // 00030018: 00030010
                0xE4, 0xFF, 0xFF, 0xFF,  // 0003001C: 00030004
            }),
            bytes);
}

// Checks that |gen| emits and only emits |expected_refs|, in order.
void CheckGen(ReferenceReader&& reader,
              const std::vector<Reference>& expected_refs) {
  for (Reference expected_ref : expected_refs) {
    auto ref = reader.GetNext();
    EXPECT_TRUE(ref.has_value());
    EXPECT_EQ(expected_ref, ref.value());
  }
  EXPECT_EQ(base::nullopt, reader.GetNext());  // Nothing should be left.
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
  ConstBufferView src1_region(&bytes1[0], bytes1.size());
  ConstBufferView src2_region(&bytes2[0], bytes2.size());
  for (rva_t src_rva : rvas) {
    for (rva_t dst_rva : rvas) {
      {
        std::vector<uint8_t> out_bytes(bytes2);
        MutableBufferView out_region(&out_bytes[0], out_bytes.size());
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
        MutableBufferView out_region(&out_bytes[0], out_bytes.size());
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

TEST(Rel32UtilsTest, ARM32CreateFindRel32) {
  constexpr offset_t kTestImageSize = 0x00100000U;
  constexpr rva_t kRvaBegin = 0x00030000U;
  TestAddressTranslator translator(kTestImageSize, kRvaBegin);

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
  ConstBufferView region(&bytes[0], bytes.size());
  // Specify rel32 locations directly, instead of parsing.
  std::vector<offset_t> rel32_locations_A24 = {0x0008U, 0x0010U, 0x0018U,
                                               0x001CU};

  // Generate everything.
  auto gen1 = ARMCreateFindRel32<ARM32Rel32Parser::AddrTraits_A24>(
      translator, region.begin(), rel32_locations_A24, 0x0000U, 0x0020U);
  CheckGen(std::move(*gen1), {{0x0008U, 0x0010U},
                              {0x0010U, 0x0014U},
                              {0x0018U, 0x0010U},
                              {0x001CU, 0x0004U}});

  // Exclude last.
  auto gen2 = ARMCreateFindRel32<ARM32Rel32Parser::AddrTraits_A24>(
      translator, region.begin(), rel32_locations_A24, 0x0000U, 0x001CU);
  CheckGen(std::move(*gen2),
           {{0x0008U, 0x0010U}, {0x0010U, 0x0014U}, {0x0018U, 0x0010U}});

  // Only find one.
  auto gen3 = ARMCreateFindRel32<ARM32Rel32Parser::AddrTraits_A24>(
      translator, region.begin(), rel32_locations_A24, 0x000CU, 0x0018U);
  CheckGen(std::move(*gen3), {{0x0010U, 0x0014U}});
}

TEST(Rel32UtilsTest, ARM32CreateReceiveRel32_Easy) {
  constexpr offset_t kTestImageSize = 0x00100000U;
  constexpr rva_t kRvaBegin = 0x00030000U;
  TestAddressTranslator translator(kTestImageSize, kRvaBegin);

  std::vector<uint8_t> bytes = {
      0xFF, 0xFF,              // 00030000: (Filler)
      0x01, 0xDE,              // 00030002: B   00030008 ; T8
      0xFF, 0xFF, 0xFF, 0xFF,  // 00030004: (Filler)
      0x01, 0xE0,              // 00030008: B   0003000E ; T11
      0xFF, 0xFF,              // 0003000A: (Filler)
      0x80, 0xF3, 0x00, 0x80,  // 0003000C: B   00030010 ; T21
  };
  MutableBufferView region(&bytes[0], bytes.size());

  auto rcv1 = ARMCreateReceiveRel32<ARM32Rel32Parser::AddrTraits_T8>(
      translator, region.begin());
  rcv1->PutNext({0x0002U, 0x0004U});
  EXPECT_EQ(0xFF, bytes[0x02]);  // 00030002: B   00030004 ; T8
  EXPECT_EQ(0xDE, bytes[0x03]);

  rcv1->PutNext({0x0002U, 0x000AU});
  EXPECT_EQ(0x02, bytes[0x02]);  // 00030002: B   0003000A ; T8
  EXPECT_EQ(0xDE, bytes[0x03]);

  auto rcv2 = ARMCreateReceiveRel32<ARM32Rel32Parser::AddrTraits_T11>(
      translator, region.begin());
  rcv2->PutNext({0x0008U, 0x0008U});
  EXPECT_EQ(0xFE, bytes[0x08]);  // 00030008: B   00030008 ; T11
  EXPECT_EQ(0xE7, bytes[0x09]);
  rcv2->PutNext({0x0008U, 0x0010U});
  EXPECT_EQ(0x02, bytes[0x08]);  // 00030008: B   00030010 ; T11
  EXPECT_EQ(0xE0, bytes[0x09]);

  auto rcv3 = ARMCreateReceiveRel32<ARM32Rel32Parser::AddrTraits_T21>(
      translator, region.begin());
  rcv3->PutNext({0x000CU, 0x000AU});
  EXPECT_EQ(0xBF, bytes[0x0C]);  // 0003000C: B   0003000A ; T21
  EXPECT_EQ(0xF7, bytes[0x0D]);
  EXPECT_EQ(0xFD, bytes[0x0E]);
  EXPECT_EQ(0xAF, bytes[0x0F]);
  rcv3->PutNext({0x000CU, 0x0010U});
  EXPECT_EQ(0x80, bytes[0x0C]);  // 0003000C: B   00030010 ; T21
  EXPECT_EQ(0xF3, bytes[0x0D]);
  EXPECT_EQ(0x00, bytes[0x0E]);
  EXPECT_EQ(0x80, bytes[0x0F]);
}

TEST(Rel32UtilsTest, ARM32CreateReceiveRel32_Hard) {
  constexpr offset_t kTestImageSize = 0x10000000U;
  constexpr rva_t kRvaBegin = 0x0C030000U;
  TestAddressTranslator translator(kTestImageSize, kRvaBegin);

  std::vector<uint8_t> bytes = {
      0xFF, 0xFF,              // 0C030000: (Filler)
      0x00, 0xF0, 0x00, 0xB8,  // 0C030002: B   0C030006 ; T24
      0xFF, 0xFF, 0xFF, 0xFF,  // 0C030006: (Filler)
      0x00, 0xF0, 0x7A, 0xE8,  // 0C03000A: BLX 0C030100 ; T24
      0xFF, 0xFF,              // 0C03000E: (Filler)
      0x00, 0xF0, 0x7A, 0xE8,  // 0C030010: BLX 0C030108 ; T24
  };
  MutableBufferView region(&bytes[0], bytes.size());

  auto rcv = ARMCreateReceiveRel32<ARM32Rel32Parser::AddrTraits_T24>(
      translator, region.begin());
  rcv->PutNext({0x0002U, 0x0000U});
  EXPECT_EQ(0xFF, bytes[0x02]);  // 0C030002: B   0C030000 ; T24
  EXPECT_EQ(0xF7, bytes[0x03]);
  EXPECT_EQ(0xFD, bytes[0x04]);
  EXPECT_EQ(0xBF, bytes[0x05]);
  rcv->PutNext({0x0002U, 0x0008U});
  EXPECT_EQ(0x00, bytes[0x02]);  // 0C030002: B   0C030008 ; T24
  EXPECT_EQ(0xF0, bytes[0x03]);
  EXPECT_EQ(0x01, bytes[0x04]);
  EXPECT_EQ(0xB8, bytes[0x05]);

  // BLX complication, with location that's not 4-byte aligned.
  rcv->PutNext({0x000AU, 0x0010U});
  EXPECT_EQ(0x00, bytes[0x0A]);  // 0C03000A: BLX 0C030010 ; T24
  EXPECT_EQ(0xF0, bytes[0x0B]);
  EXPECT_EQ(0x02, bytes[0x0C]);
  EXPECT_EQ(0xE8, bytes[0x0D]);
  rcv->PutNext({0x000AU, 0x0100U});
  EXPECT_EQ(0x00, bytes[0x0A]);  // 0C03000A: BLX 0C030100 ; T24
  EXPECT_EQ(0xF0, bytes[0x0B]);
  EXPECT_EQ(0x7A, bytes[0x0C]);
  EXPECT_EQ(0xE8, bytes[0x0D]);
  rcv->PutNext({0x000AU, 0x0000U});
  EXPECT_EQ(0xFF, bytes[0x0A]);  // 0C03000A: BLX 0C030000 ; T24
  EXPECT_EQ(0xF7, bytes[0x0B]);
  EXPECT_EQ(0xFA, bytes[0x0C]);
  EXPECT_EQ(0xEF, bytes[0x0D]);

  // BLX complication, with location that's 4-byte aligned.
  rcv->PutNext({0x0010U, 0x0010U});
  EXPECT_EQ(0xFF, bytes[0x10]);  // 0C030010: BLX 0C030010 ; T24
  EXPECT_EQ(0xF7, bytes[0x11]);
  EXPECT_EQ(0xFE, bytes[0x12]);
  EXPECT_EQ(0xEF, bytes[0x13]);
  rcv->PutNext({0x0010U, 0x0108U});
  EXPECT_EQ(0x00, bytes[0x10]);  // 0C030010: BLX 0C030108 ; T24
  EXPECT_EQ(0xF0, bytes[0x11]);
  EXPECT_EQ(0x7A, bytes[0x12]);
  EXPECT_EQ(0xE8, bytes[0x13]);
}

// Test BLX encoding A2, which is an ARM instruction that switches to THUMB2,
// and therefore should have 2-byte alignment.
TEST(Rel32UtilsTest, ARM32SwitchToThumb2) {
  constexpr offset_t kTestImageSize = 0x10000000U;
  constexpr rva_t kRvaBegin = 0x08030000U;
  TestAddressTranslator translator(kTestImageSize, kRvaBegin);

  std::vector<uint8_t> bytes = {
      0xFF, 0xFF, 0x00, 0x00,  // 08030000: (Filler)
      0x00, 0x00, 0x00, 0xFA,  // 08030004: BLX 0803000C ; A24
  };
  MutableBufferView region(&bytes[0], bytes.size());

  auto rcv = ARMCreateReceiveRel32<ARM32Rel32Parser::AddrTraits_A24>(
      translator, region.begin());

  // To location that's 4-byte aligned.
  rcv->PutNext({0x0004U, 0x0100U});
  EXPECT_EQ(0x3D, bytes[0x04]);  // 08030004: BLX 08030100 ; A24
  EXPECT_EQ(0x00, bytes[0x05]);
  EXPECT_EQ(0x00, bytes[0x06]);
  EXPECT_EQ(0xFA, bytes[0x07]);

  // To location that's 2-byte aligned but not 4-byte aligned.
  rcv->PutNext({0x0004U, 0x0052U});
  EXPECT_EQ(0x11, bytes[0x04]);  // 08030004: BLX 08030052 ; A24
  EXPECT_EQ(0x00, bytes[0x05]);
  EXPECT_EQ(0x00, bytes[0x06]);
  EXPECT_EQ(0xFB, bytes[0x07]);

  // Clean slate code.
  rcv->PutNext({0x0004U, 0x000CU});
  EXPECT_EQ(0x00, bytes[0x04]);  // 08030004: BLX 0803000C ; A24
  EXPECT_EQ(0x00, bytes[0x05]);
  EXPECT_EQ(0x00, bytes[0x06]);
  EXPECT_EQ(0xFA, bytes[0x07]);
}

TEST(Rel32UtilsTest, ARM32CopyDisplacement) {
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

TEST(Rel32UtilsTest, AArch64CreateFindRel32) {
  constexpr offset_t kTestImageSize = 0x00100000U;
  constexpr rva_t kRvaBegin = 0x00030000U;
  TestAddressTranslator translator(kTestImageSize, kRvaBegin);

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
  MutableBufferView region(&bytes[0], bytes.size());

  // Generate Immd26. We specify rel32 locations directly.
  std::vector<offset_t> rel32_locations_Immd26 = {0x0008U};
  auto gen1 = ARMCreateFindRel32<AArch64Rel32Parser::AddrTraits_Immd26>(
      translator, region.begin(), rel32_locations_Immd26, 0x0000U, 0x0020U);
  CheckGen(std::move(*gen1), {{0x0008U, 0x0010U}});

  // Generate Immd19.
  std::vector<offset_t> rel32_locations_Immd19 = {0x0010U, 0x0018U};
  auto gen2 = ARMCreateFindRel32<AArch64Rel32Parser::AddrTraits_Immd19>(
      translator, region.begin(), rel32_locations_Immd19, 0x0000U, 0x0020U);
  CheckGen(std::move(*gen2), {{0x0010U, 0x0014U}, {0x0018U, 0x0010U}});

  // Generate Immd14.
  std::vector<offset_t> rel32_locations_Immd14 = {0x001CU};
  auto gen3 = ARMCreateFindRel32<AArch64Rel32Parser::AddrTraits_Immd14>(
      translator, region.begin(), rel32_locations_Immd14, 0x0000U, 0x0020U);
  CheckGen(std::move(*gen3), {{0x001CU, 0x0004U}});
}

TEST(Rel32UtilsTest, AArch64CopyDisplacement) {
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
