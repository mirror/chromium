// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/arm_utils.h"

namespace zucchini {

/******** ARM32Rel32Parser ********/

bool ARM32Rel32Parser::DecodeA24(uint32_t code32, arm_disp_t* disp) {
  // Notation: Like in manual, "Code" is listed high-order bit first.
  // "Displacement" is represented in 32-bit integer and also listed high-order
  // bit first.

  // Handle multiple instructions. Let cccc != 1111:
  // B encoding A1:
  //   Code:         cccc1010 Siiiiiii iiiiiiii iiiiiiii
  //   Displacement: SSSSSSSi iiiiiiii iiiiiiii iiiiii00
  // BL encoding A1:
  //   Code:         cccc1011 Siiiiiii iiiiiiii iiiiiiii
  //   Displacement: SSSSSSSi iiiiiiii iiiiiiii iiiiii00
  // BLX encoding A2:
  //   Code:         1111101H Siiiiiii iiiiiiii iiiiiiii
  //   Displacement: SSSSSSSi iiiiiiii iiiiiiii iiiiiiH0
  uint8_t bits = GetUnsignedBits<24, 27>(code32);
  if (bits == 0xA || bits == 0xB) {  // B, BL, or BLX.
    *disp = GetSignedBits<0, 23>(code32) << 2;
    uint8_t cond = GetUnsignedBits<28, 31>(code32);
    if (cond == 0xF) {  // BLX.
      uint32_t H = GetBit<24>(code32);
      *disp |= H << 1;
      align_by_ = 2;
    } else {
      align_by_ = 4;
    }
    return true;
  }
  return false;
}

bool ARM32Rel32Parser::EncodeA24(arm_disp_t disp, uint32_t* code32) {
  uint32_t t = *code32;
  uint8_t bits = GetUnsignedBits<24, 27>(t);
  if (bits == 0xA || bits == 0xB) {
    // B, BL, or BLX.
    if (!SignedFit<26>(disp))  // Detect overflow.
      return false;
    uint8_t cond = GetUnsignedBits<28, 31>(t);
    if (cond == 0xF) {
      if (disp % 2)  // BLX (encoding A2) requires 2-byte alignment.
        return false;
      uint32_t H = (disp >> 1) & 1;
      t = (t & 0xFEFFFFFF) | (H << 24);
      align_by_ = 2;
    } else {
      if (disp % 4)  // B and BL require 4-byte alignment.
        return false;
      align_by_ = 4;
    }
    t = (t & 0xFF000000) | ((disp >> 2) & 0x00FFFFFF);
    *code32 = t;
    return true;
  }
  return false;
}

bool ARM32Rel32Parser::DecodeT8(uint16_t code16, arm_disp_t* disp) {
  if ((code16 & 0xF000) == 0xD000 && (code16 & 0x0F00) != 0x0F00) {
    // B encoding T1:
    //   Code:         1101cccc Siiiiiii
    //   Displacement: SSSSSSSS SSSSSSSS SSSSSSSS iiiiiii0
    *disp = GetSignedBits<0, 7>(code16) << 1;
    align_by_ = 2;
    return true;
  }
  return false;
}

bool ARM32Rel32Parser::EncodeT8(arm_disp_t disp, uint16_t* code16) {
  uint16_t t = *code16;
  if ((t & 0xF000) == 0xD000 && (t & 0x0F00) != 0x0F00) {
    if (disp % 2)  // Require 2-byte alignment.
      return false;
    if (!SignedFit<9>(disp))  // Detect overflow.
      return false;
    t = (t & 0xFF00) | ((disp >> 1) & 0x00FF);
    *code16 = t;
    align_by_ = 2;
    return true;
  }
  return false;
}

bool ARM32Rel32Parser::DecodeT11(uint16_t code16, arm_disp_t* disp) {
  if ((code16 & 0xF800) == 0xE000) {
    // B encoding T2:
    //   Code:         11100Sii iiiiiiii
    //   Displacement: SSSSSSSS SSSSSSSS SSSSSiii iiiiiii0
    *disp = GetSignedBits<0, 10>(code16) << 1;
    align_by_ = 2;
    return true;
  }
  return false;
}

bool ARM32Rel32Parser::EncodeT11(arm_disp_t disp, uint16_t* code16) {
  uint16_t t = *code16;
  if ((t & 0xF800) == 0xE000) {
    if (disp % 2)  // Require 2-byte alignment.
      return false;
    if (!SignedFit<12>(disp))  // Detect overflow.
      return false;
    t = (t & 0xF800) | ((disp >> 1) & 0x07FF);
    *code16 = t;
    align_by_ = 2;
    return true;
  }
  return false;
}

bool ARM32Rel32Parser::DecodeT21(uint32_t code32, arm_disp_t* disp) {
  if ((code32 & 0xF800D000) == 0xF0008000 &&
      (code32 & 0x03C00000) != 0x03C00000) {
    // B encoding T3 (note peculiar reversal of J1 and J2).
    //   Code:         11110Scc cciiiiii 10(J1)0(J2)jjj jjjjjjjj
    //   Displacement: SSSSSSSS SSSS(J2)(J1)ii iiiijjjj jjjjjjj0
    uint32_t imm11 = GetUnsignedBits<0, 10>(code32);
    uint32_t J2 = GetBit<11>(code32);
    uint32_t J1 = GetBit<13>(code32);
    uint32_t imm6 = GetUnsignedBits<16, 21>(code32);
    uint32_t S = GetBit<26>(code32);
    uint32_t t = (imm6 << 12) | (imm11 << 1);
    t |= (S << 20) | (J2 << 19) | (J1 << 18);
    *disp = SignExtend<20, int32_t>(t);
    align_by_ = 2;
    return true;
  }
  return false;
}

bool ARM32Rel32Parser::EncodeT21(arm_disp_t disp, uint32_t* code32) {
  uint32_t t = *code32;
  if ((t & 0xF800D000) == 0xF0008000 && (t & 0x03C00000) != 0x03C00000) {
    if (disp % 2)  // Require 2-byte alignment.
      return false;
    if (!SignedFit<21>(disp))  // Detect overflow.
      return false;
    uint32_t S = GetBit<20>(disp);
    uint32_t J2 = GetBit<19>(disp);
    uint32_t J1 = GetBit<18>(disp);
    uint32_t imm6 = GetUnsignedBits<12, 17>(disp);
    uint32_t imm11 = GetUnsignedBits<1, 11>(disp);
    t &= 0xFBC0D000;
    t |= (S << 26) | (imm6 << 16) | (J1 << 13) | (J2 << 11) | imm11;
    *code32 = t;
    align_by_ = 2;
    return true;
  }
  return false;
}

bool ARM32Rel32Parser::DecodeT24(uint32_t code32, arm_disp_t* disp) {
  uint32_t bits = code32 & 0xF800D000;
  if (bits == 0xF0009000 || bits == 0xF000D000 || bits == 0xF000C000) {
    // Let I1 = J1 ^ S ^ 1, I2 = J2 ^ S ^ 1.
    // B encoding T4:
    //   Code:         11110Sii iiiiiiii 10(J1)1(J2)jjj jjjjjjjj
    //   Displacement: SSSSSSSS (I1)(I2)iiiiii iiiijjjj jjjjjjj0
    // BL encoding T1:
    //   Code:         11110Sii iiiiiiii 11(J1)1(J2)jjj jjjjjjjj
    //   Displacement: SSSSSSSS (I1)(I2)iiiiii iiiijjjj jjjjjjj0
    // BLX encoding T2: H should be 0:
    //   Code:         11110Sii iiiiiiii 11(J1)0(J2)jjj jjjjjjjH
    //   Displacement: SSSSSSSS (I1)(I2)iiiiii iiiijjjj jjjjjjH0
    uint32_t imm11 = GetUnsignedBits<0, 10>(code32);
    uint32_t J2 = GetBit<11>(code32);
    uint32_t J1 = GetBit<13>(code32);
    uint32_t imm10 = GetUnsignedBits<16, 25>(code32);
    uint32_t S = GetBit<26>(code32);
    uint32_t t = (imm10 << 12) | (imm11 << 1);
    t |= (S << 24) | ((J1 ^ S ^ 1) << 23) | ((J2 ^ S ^ 1) << 22);
    t = SignExtend<24, int32_t>(t);
    // BLX encoding T2 requires final target to be 4-byte aligned. If
    // |instr_rva_| is not aligned, then we round down to the nearest 4-byte
    // aligned address, This is applied to |t| *after* clipping.
    int temp_align_by = 2;
    if (bits == 0xF000C000) {
      uint32_t H = code32 & 1;
      if (H)
        return false;  // Illegal instruction: H must be 0.
      temp_align_by = 4;
    }
    *disp = static_cast<int32_t>(t);
    align_by_ = temp_align_by;
    return true;
  }
  return false;
}

bool ARM32Rel32Parser::EncodeT24(arm_disp_t disp, uint32_t* code32) {
  uint32_t t = *code32;
  uint32_t bits = t & 0xF800D000;
  if (bits == 0xF0009000 || bits == 0xF000D000 || bits == 0xF000C000) {
    if (disp % 2)  // Require 2-byte alignment.
      return false;
    // BLX encoding T2 requires H == 0, and that |disp| results in |target_rva|
    // with a 4-byte aligned address.
    int temp_align_by = 2;
    if (bits == 0xF000C000) {
      uint32_t H = GetBit<1>(disp);
      if (H)
        return false;  // Illegal |disp|: H must be 0.
      temp_align_by = 4;
    }
    if (!SignedFit<25>(disp))  // Detect overflow.
      return false;
    uint32_t imm11 = GetUnsignedBits<1, 11>(disp);
    uint32_t imm10 = GetUnsignedBits<12, 21>(disp);
    uint32_t I2 = GetBit<22>(disp);
    uint32_t I1 = GetBit<23>(disp);
    uint32_t S = GetBit<24>(disp);
    t &= 0xF800D000;
    t |= (S << 26) | (imm10 << 16) | ((I1 ^ S ^ 1) << 13) |
         ((I2 ^ S ^ 1) << 11) | imm11;
    *code32 = t;
    align_by_ = temp_align_by;
    return true;
  }
  return false;
}

/******** AArch64Rel32Parser ********/

bool AArch64Rel32Parser::DecodeImmd14(uint32_t code32, arm_disp_t* disp) {
  // TBZ:
  //   Code:         b0110110 bbbbbSii iiiiiiii iiittttt
  //   Displacement: SSSSSSSS SSSSSSSS Siiiiiii iiiiii00
  // TBNZ:
  //   Code:         b0110111 bbbbbSii iiiiiiii iiittttt
  //   Displacement: SSSSSSSS SSSSSSSS Siiiiiii iiiiii00
  uint32_t bits = code32 & 0x7F000000;
  if (bits == 0x36000000 || bits == 0x37000000) {
    *disp = GetSignedBits<5, 18>(code32) << 2;
    return true;
  }
  return false;
}

bool AArch64Rel32Parser::EncodeImmd14(arm_disp_t disp, uint32_t* code32) {
  uint32_t t = *code32;
  uint32_t bits = t & 0x7F000000;
  if (bits == 0x36000000 || bits == 0x37000000) {
    if (disp % 4)  // Require 4-byte alignment.
      return false;
    if (!SignedFit<16>(disp))  // Detect overflow.
      return false;
    uint32_t imm14 = GetUnsignedBits<2, 15>(disp);
    t &= 0xFFF8001F;
    t |= imm14 << 5;
    *code32 = t;
    return true;
  }
  return false;
}

bool AArch64Rel32Parser::DecodeImmd19(uint32_t code32, arm_disp_t* disp) {
  // B.cond:
  //   Code:         01010100 Siiiiiii iiiiiiii iii0cccc
  //   Displacement: SSSSSSSS SSSSiiii iiiiiiii iiiiii00
  // CBZ:
  //   Code:         z0110100 Siiiiiii iiiiiiii iiittttt
  //   Displacement: SSSSSSSS SSSSiiii iiiiiiii iiiiii00
  // CBNZ:
  //   Code:         z0110101 Siiiiiii iiiiiiii iiittttt
  //   Displacement: SSSSSSSS SSSSiiii iiiiiiii iiiiii00
  uint32_t bits1 = code32 & 0xFF000010;
  uint32_t bits2 = code32 & 0x7F000000;
  if (bits1 == 0x54000000 || bits2 == 0x34000000 || bits2 == 0x35000000) {
    *disp = GetSignedBits<5, 23>(code32) << 2;
    return true;
  }
  return false;
}

bool AArch64Rel32Parser::EncodeImmd19(arm_disp_t disp, uint32_t* code32) {
  uint32_t t = *code32;
  uint32_t bits1 = t & 0xFF000010;
  uint32_t bits2 = t & 0x7F000000;
  if (bits1 == 0x54000000 || bits2 == 0x34000000 || bits2 == 0x35000000) {
    if (disp % 4)  // Require 4-byte alignment.
      return false;
    if (!SignedFit<21>(disp))  // Detect overflow.
      return false;
    uint32_t imm19 = GetUnsignedBits<2, 20>(disp);
    t &= 0xFF00001F;
    t |= imm19 << 5;
    *code32 = t;
    return true;
  }
  return false;
}

bool AArch64Rel32Parser::DecodeImmd26(uint32_t code32, arm_disp_t* disp) {
  // B:
  //   Code:         000101Si iiiiiiii iiiiiiii iiiiiiii
  //   Displacement: SSSSSiii iiiiiiii iiiiiiii iiiiii00
  // BL:
  //   Code:         100101Si iiiiiiii iiiiiiii iiiiiiii
  //   Displacement: SSSSSiii iiiiiiii iiiiiiii iiiiii00
  uint32_t bits = code32 & 0xFC000000;
  if (bits == 0x14000000 || bits == 0x94000000) {
    *disp = GetSignedBits<0, 25>(code32) << 2;
    return true;
  }
  return false;
}

bool AArch64Rel32Parser::EncodeImmd26(arm_disp_t disp, uint32_t* code32) {
  uint32_t t = *code32;
  uint32_t bits = t & 0xFC000000;
  if (bits == 0x14000000 || bits == 0x94000000) {
    if (disp % 4)  // Require 4-byte alignment.
      return false;
    if (!SignedFit<28>(disp))  // Detect overflow.
      return false;
    uint32_t imm26 = GetUnsignedBits<2, 27>(disp);
    t &= 0xFC000000;
    t |= imm26;
    *code32 = t;
    return true;
  }
  return false;
}

}  // namespace zucchini
