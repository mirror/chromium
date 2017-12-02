// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_ARM_UTILS_H_
#define CHROME_INSTALLER_ZUCCHINI_ARM_UTILS_H_

#include <stddef.h>
#include <stdint.h>

#include "base/logging.h"
#include "base/macros.h"
#include "chrome/installer/zucchini/address_translator.h"
#include "chrome/installer/zucchini/buffer_view.h"
#include "chrome/installer/zucchini/image_utils.h"

namespace zucchini {

// Definitions:
// - |instr_rva|: Instruction RVA: The RVA where an instruction is located. In
//   ARM mode this is 4-byte aligned. In THUMB2 mode this is 2-byte aligned.
// - |code|: Instruction code: ARM instruction code as seen in manual. In ARM
//   mode this 32-bit. In THUMB2 mode, this may be 16-bit or 32-bit.
// - |disp|: Displacement: For branch instructions (e.g.: B, BL, BLX, and
//   conditional versions) this is the value encoded in instruction bytes.
// - PC: Program Counter, which is displaced from |instr_rva| by 8 bytes in ARM
//   mode, or 4 bytes in THUMB2 mode.
// - |target_rva|: Target RVA: The RVA targetted by a branch instruction.
//
// These are related by:
//   |code| = Fetch(image data at offset(|instr_rva|)).
//   |disp| = Decode(|code|).
//   PC = |instr_rva| + {8 in ARM mode, 4 in THUMB2 mode}.
//   |target_rva| = PC + |disp| - (see "BLX complication" below)
//
// Example 1 (ARM mode):
//   00103050: 00 01 02 EA    B     00183458
//   |instr_rva| = 0x00103050  (4-byte aligned).
//   |code| = 0xEA020100  (little endian fetched from data).
//   |disp| = 0x00080400  (decoded from |code| with A24 -> B encoding T1).
//   PC = |instr_rva| + 8 = 0x00103058  (ARM mode).
//   |target_rva| = PC + |disp| = 0x00183458.
//
// Example 2 (THUMB2 mode):
//   001030A2: 00 F0 01 FA    BL    001034A8
//   |instr_rva| = 0x001030A2  (2-byte aligned).
//   |code| = 0xF000FA01  (special THUMB2 mode data fetch method).
//   |disp| = 0x00000402  (decoded from |code| with T24 -> BL encoding T1).
//   PC = |instr_rva| + 4 = 0x001030A6  (THUMB2 mode).
//   |target_rva| = PC + |disp| = 0x001034A8.

// BLX complication: BLX encoding T2 transits mode from THUMB2 to ARM. Therefore
// |target_rva| must be 4-byte aligned; it's not just PC + |disp|. In THUMB2
// encoding, |disp| is required to be multiple of 4 (so H = 0, where H is the
// bit corresponding to 2). |target_rva| becomes PC + |disp| rounded down to
// the nearest 4-byte aligned address. We have two alternatives to handle this
// complexity (this affects |disp|'s definition):
// (1) Handle in {|code|, |disp|} conversion: Let |disp| be |target_rva| - PC
//     For BLX encoding T2, we'd examine |instr_rva| % 4 and adjust accordingly.
// (2) Handle {|disp|, |target_rva|} conversion: Let |disp| be the value stored
//     in |code|. Computation involving |target_rva| would require |instr_rva|
//     and prior |code| extraction, from which we deduce expected target
//     alignment (4 for ARM mode; 2 for THUMB2 mode, but 4 for BLX encoding T2)
//     and adjust accordingly.
// We adopt (2). This simplifies the definition of |disp|, and testing for
// Decode/Encode. The cost is that ARM32Rel32Parser becomes stateful; it stores
// |align_by_| in order to convert between {|disp|, |target_rva|}.

using arm_disp_t = int32_t;

// Given THUMB2 instruction |code16|, returns 2 if it's from a 16-bit THUMB2
// instruction, or 4 if it's from a 32-bit THUMB2 instruction.
inline int GetTHUMB2InstructionSize(uint16_t code16) {
  return ((code16 & 0xF000) == 0xF000 || (code16 & 0xF800) == 0xE800) ? 4 : 2;
}

// A stateful parser for ARM mode and THUMB2 mode that wraps |instr_rva_|, and
// implements accessors to compute |code|, |disp|, and |target_rva|. After
// successful Decode/Encode, |align_by_| is updated. This in turn affects
// {|disp|, |target_rva|} conversion.
class ARM32Rel32Parser {
 public:
  static const int kUnintializedAlignBy = 0U;

  using THIS = ARM32Rel32Parser;

  // Rel32 address types enumeration.
  enum AddrType : uint8_t {
    ADDR_NONE = 0xFF,
    ADDR_A24 = 0,
    ADDR_T8,
    ADDR_T11,
    ADDR_T21,
    ADDR_T24,
    NUM_ADDR_TYPE
  };

  // Traits for rel32 address types, which form collections of strategies to
  // process each rel32 address type.
  template <typename CODE_T,
            AddrType TYPE,
            CODE_T (ARM32Rel32Parser::*FETCH)(ConstBufferView::const_iterator)
                const,
            void (ARM32Rel32Parser::*STORE)(MutableBufferView::iterator, CODE_T)
                const,
            bool (ARM32Rel32Parser::*DECODE)(CODE_T, arm_disp_t*),
            bool (ARM32Rel32Parser::*ENCODE)(arm_disp_t, CODE_T*),
            rva_t (ARM32Rel32Parser::*TARGET_RVA_FROM_DISP)(arm_disp_t) const,
            arm_disp_t (ARM32Rel32Parser::*DISP_FROM_TARGET_RVA)(rva_t) const>
  class AddrTraits {
   public:
    using code_t = CODE_T;
    typedef ARM32Rel32Parser Parser;
    static constexpr AddrType addr_type = TYPE;
    static constexpr CODE_T (ARM32Rel32Parser::*Fetch)(
        ConstBufferView::const_iterator) const = FETCH;
    static constexpr void (ARM32Rel32Parser::*Store)(
        MutableBufferView::iterator,
        CODE_T) const = STORE;
    static constexpr bool (ARM32Rel32Parser::*Decode)(CODE_T,
                                                      arm_disp_t*) = DECODE;
    static constexpr bool (ARM32Rel32Parser::*Encode)(arm_disp_t,
                                                      CODE_T*) = ENCODE;
    static constexpr rva_t (ARM32Rel32Parser::*TargetRvaFromDisp)(
        arm_disp_t) const = TARGET_RVA_FROM_DISP;
    static constexpr arm_disp_t (ARM32Rel32Parser::*DispFromTargetRva)(
        rva_t) const = DISP_FROM_TARGET_RVA;
  };

  explicit ARM32Rel32Parser(rva_t instr_rva)
      : instr_rva_(instr_rva), align_by_(kUnintializedAlignBy) {}

  // Fetches the 32-bit ARM instruction |code| at |cur| from image data.
  inline uint32_t FetchARMCode32(ConstBufferView::const_iterator cur) const {
    return *reinterpret_cast<const uint32_t*>(cur);
  }

  // Fetches the 16-bit THUMB2 instruction |code| at |cur| from image data.
  inline uint16_t FetchTHUMB2Code16(ConstBufferView::const_iterator cur) const {
    return *reinterpret_cast<const uint16_t*>(cur);
  }

  // Fetches the next 32-bit THUMB2 instruction |code| at |cur| from image data.
  inline uint32_t FetchTHUMB2Code32(ConstBufferView::const_iterator cur) const {
    // By convension, 32-bit THUMB2 instructions are written (as seen later) as:
    //   [byte3, byte2, byte1, byte0].
    // However (assuming little-endian ARM) the in-memory representation is
    //   [byte2, byte3, byte0, byte1].
    return (static_cast<uint32_t>(*reinterpret_cast<const uint16_t*>(cur))
            << 16) |
           *reinterpret_cast<const uint16_t*>(cur + 2);
  }

  // Stores the 32-bit ARM instruction |code| to image data at |cur|.
  inline void StoreARMCode32(MutableBufferView::iterator cur,
                             uint32_t code) const {
    *reinterpret_cast<uint32_t*>(cur) = code;
  }

  // Stores the 16-bit THUMB2 instruction |code| to image data at |cur|.
  inline void StoreTHUMB2Code16(MutableBufferView::iterator cur,
                                uint16_t code) const {
    *reinterpret_cast<uint16_t*>(cur) = code;
  }

  // Stores the next 32-bit THUMB2 instruction |code| to image data at |cur|.
  inline void StoreTHUMB2Code32(MutableBufferView::iterator cur,
                                uint32_t code) const {
    *reinterpret_cast<uint16_t*>(cur) = static_cast<uint16_t>(code >> 16);
    *reinterpret_cast<uint16_t*>(cur + 2) =
        static_cast<uint16_t>(code & 0xFFFF);
  }

  // Decode*() and Encode*() functions convert between |code| (16-bit or 32-bit)
  // and |code| for various address types. These affect |last_encoding_|.
  // - Decode*() determines whether |code16| or |code32| is a branch instruction
  //   of a given address type. If so, then it extracts |disp| and returns true.
  //   Otherwise returns false.
  // - Encode*() determines whether |*code16| or |*code32| is a branch
  //   instruction of a given rel32 address type, and whether it can accommodate
  //   |disp|. If so then reencodes |*code32| with |disp| and returns true.
  //   Otherwise returns false.
  bool DecodeA24(uint32_t code32, arm_disp_t* disp);
  bool EncodeA24(arm_disp_t disp, uint32_t* code32);

  bool DecodeT8(uint16_t code16, arm_disp_t* disp);
  bool EncodeT8(arm_disp_t disp, uint16_t* code16);

  bool DecodeT11(uint16_t code16, arm_disp_t* disp);
  bool EncodeT11(arm_disp_t disp, uint16_t* code16);

  bool DecodeT21(uint32_t code32, arm_disp_t* disp);
  bool EncodeT21(arm_disp_t disp, uint32_t* code32);

  bool DecodeT24(uint32_t code32, arm_disp_t* disp);
  bool EncodeT24(arm_disp_t disp, uint32_t* code32);

  // Computes |target_rva| from |instr_rva_| and |disp| in ARM mode.
  inline rva_t GetARMTargetRvaFromDisplacement(arm_disp_t disp) const {
    rva_t ret = static_cast<rva_t>(instr_rva_ + 8 + disp);
    // Align down.
    DCHECK_NE(align_by_, kUnintializedAlignBy);
    return ret - (ret & static_cast<rva_t>(align_by_ - 1));
  }

  // Computes |target_rva| from |instr_rva_| and |disp| in THUMB2 mode.
  inline rva_t GetTHUMB2TargetRvaFromDisplacement(arm_disp_t disp) const {
    rva_t ret = static_cast<rva_t>(instr_rva_ + 4 + disp);
    // Align down.
    DCHECK_NE(align_by_, kUnintializedAlignBy);
    return ret - (ret & static_cast<rva_t>(align_by_ - 1));
  }

  // Computes |disp| from |instr_rva_| and |target_rva| in ARM mode.
  inline arm_disp_t GetARMDisplacementFromTargetRva(rva_t target_rva) const {
    // Assumes that |instr_rva_ + 8| does not overflow.
    arm_disp_t ret = static_cast<arm_disp_t>(target_rva) -
                     static_cast<arm_disp_t>(instr_rva_ + 8);
    // Align up.
    DCHECK_NE(align_by_, kUnintializedAlignBy);
    return ret + ((-ret) & static_cast<arm_disp_t>(align_by_ - 1));
  }

  // Computes |disp| from |instr_rva_| and |target_rva| in THUMB2 mode.
  inline arm_disp_t GetTHUMB2DisplacementFromTargetRva(rva_t target_rva) const {
    // Assumes that |instr_rva_ + 4| does not overflow.
    arm_disp_t ret = static_cast<arm_disp_t>(target_rva) -
                     static_cast<arm_disp_t>(instr_rva_ + 4);
    // Align up.
    DCHECK_NE(align_by_, kUnintializedAlignBy);
    return ret + ((-ret) & static_cast<arm_disp_t>(align_by_ - 1));
  }

  inline bool CheckAlign(rva_t rva) const { return !(rva & (align_by_ - 1)); }

  // Strategies to process each rel32 address type.
  using AddrTraits_A24 = AddrTraits<uint32_t,
                                    ADDR_A24,
                                    &THIS::FetchARMCode32,
                                    &THIS::StoreARMCode32,
                                    &THIS::DecodeA24,
                                    &THIS::EncodeA24,
                                    &THIS::GetARMTargetRvaFromDisplacement,
                                    &THIS::GetARMDisplacementFromTargetRva>;
  using AddrTraits_T8 = AddrTraits<uint16_t,
                                   ADDR_T8,
                                   &THIS::FetchTHUMB2Code16,
                                   &THIS::StoreTHUMB2Code16,
                                   &THIS::DecodeT8,
                                   &THIS::EncodeT8,
                                   &THIS::GetTHUMB2TargetRvaFromDisplacement,
                                   &THIS::GetTHUMB2DisplacementFromTargetRva>;
  using AddrTraits_T11 = AddrTraits<uint16_t,
                                    ADDR_T11,
                                    &THIS::FetchTHUMB2Code16,
                                    &THIS::StoreTHUMB2Code16,
                                    &THIS::DecodeT11,
                                    &THIS::EncodeT11,
                                    &THIS::GetTHUMB2TargetRvaFromDisplacement,
                                    &THIS::GetTHUMB2DisplacementFromTargetRva>;
  using AddrTraits_T21 = AddrTraits<uint32_t,
                                    ADDR_T21,
                                    &THIS::FetchTHUMB2Code32,
                                    &THIS::StoreTHUMB2Code32,
                                    &THIS::DecodeT21,
                                    &THIS::EncodeT21,
                                    &THIS::GetTHUMB2TargetRvaFromDisplacement,
                                    &THIS::GetTHUMB2DisplacementFromTargetRva>;
  using AddrTraits_T24 = AddrTraits<uint32_t,
                                    ADDR_T24,
                                    &THIS::FetchTHUMB2Code32,
                                    &THIS::StoreTHUMB2Code32,
                                    &THIS::DecodeT24,
                                    &THIS::EncodeT24,
                                    &THIS::GetTHUMB2TargetRvaFromDisplacement,
                                    &THIS::GetTHUMB2DisplacementFromTargetRva>;

 private:
  // The instruction RVA, which is used to:
  // - Compute |target_rva|, given |disp|.
  // - Decode/Encode |code| in special cases. Specifically, this affects BLX
  //   encoding T2 because it requires 4-byte alignment.
  const rva_t instr_rva_;

  // Alignment requirement for |target_rva|. We assign this in Decode/Encode
  // functions, and used this in {|disp|, |target_rva|} conversion. Once
  // assigned |align_by_| is 2 or 4. This allows us to implement |x % align_by_|
  // with |x & (align_by_ - 1)|.
  int align_by_;

  DISALLOW_COPY_AND_ASSIGN(ARM32Rel32Parser);
};

// Parser for AArch64, which is simpler than 32-bit ARM. Although pointers are
// 64-bit, displacements are within 32-bit.
class AArch64Rel32Parser {
 public:
  using THIS = AArch64Rel32Parser;

  // Rel64 address types enumeration.
  enum AddrType : uint8_t {
    ADDR_NONE = 0xFF,
    ADDR_IMMD14 = 0,
    ADDR_IMMD19,
    ADDR_IMMD26,
    NUM_ADDR_TYPE
  };

  // Although RVA for 64-bit architecture can be 64-bit in length, we make the
  // bold assumption that for ELF images that RVA will stay nicely in 32-bit!
  explicit AArch64Rel32Parser(rva_t instr_rva) : instr_rva_(instr_rva) {}

  inline uint32_t Fetch(ConstBufferView::const_iterator cur) const {
    return *reinterpret_cast<const uint32_t*>(cur);
  }

  inline void Store(MutableBufferView::iterator cur, uint32_t code) const {
    *reinterpret_cast<uint32_t*>(cur) = code;
  }

  bool DecodeImmd14(uint32_t code32, arm_disp_t* disp);
  bool EncodeImmd14(arm_disp_t disp, uint32_t* code32);

  bool DecodeImmd19(uint32_t code32, arm_disp_t* disp);
  bool EncodeImmd19(arm_disp_t disp, uint32_t* code32);

  bool DecodeImmd26(uint32_t code32, arm_disp_t* disp);
  bool EncodeImmd26(arm_disp_t disp, uint32_t* code32);

  inline rva_t GetTargetRvaFromDisplacement(arm_disp_t disp) const {
    return static_cast<rva_t>(instr_rva_ + disp);
  }

  inline arm_disp_t GetDisplacementFromTargetRva(rva_t target_rva) const {
    return static_cast<arm_disp_t>(target_rva - instr_rva_);
  }

  // Traits for rel32 (technically rel64, but we assume numbers will be small
  // enough) address types, which form collections of strategies to process
  // each rel32 address type.
  template <AddrType TYPE,
            bool (AArch64Rel32Parser::*DECODE)(uint32_t, arm_disp_t*),
            bool (AArch64Rel32Parser::*ENCODE)(arm_disp_t, uint32_t*)>
  class AddrTraits {
   public:
    using code_t = uint32_t;
    typedef AArch64Rel32Parser Parser;
    static constexpr AddrType addr_type = TYPE;

    static constexpr uint32_t (AArch64Rel32Parser::*Fetch)(
        ConstBufferView::const_iterator) const = &AArch64Rel32Parser::Fetch;
    static constexpr void (AArch64Rel32Parser::*Store)(
        MutableBufferView::iterator,
        uint32_t) const = &AArch64Rel32Parser::Store;
    static constexpr bool (AArch64Rel32Parser::*Decode)(uint32_t,
                                                        arm_disp_t*) = DECODE;
    static constexpr bool (AArch64Rel32Parser::*Encode)(arm_disp_t,
                                                        uint32_t*) = ENCODE;
    static constexpr rva_t (AArch64Rel32Parser::*TargetRvaFromDisp)(
        arm_disp_t) const = &AArch64Rel32Parser::GetTargetRvaFromDisplacement;
    static constexpr arm_disp_t (AArch64Rel32Parser::*DispFromTargetRva)(
        rva_t) const = &AArch64Rel32Parser::GetDisplacementFromTargetRva;
  };

  // Strategies to process each rel32 address type.
  using AddrTraits_Immd14 =
      AddrTraits<ADDR_IMMD14, &THIS::DecodeImmd14, &THIS::EncodeImmd14>;
  using AddrTraits_Immd19 =
      AddrTraits<ADDR_IMMD19, &THIS::DecodeImmd19, &THIS::EncodeImmd19>;
  using AddrTraits_Immd26 =
      AddrTraits<ADDR_IMMD26, &THIS::DecodeImmd26, &THIS::EncodeImmd26>;

 private:
  const rva_t instr_rva_;

  DISALLOW_COPY_AND_ASSIGN(AArch64Rel32Parser);
};

}  // namespace zucchini

#endif  // CHROME_INSTALLER_ZUCCHINI_ARM_UTILS_H_
