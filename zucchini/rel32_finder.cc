// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "zucchini/rel32_finder.h"

#include <algorithm>
#include <cassert>

namespace zucchini {

/******** Rel32Finder ********/

// The main range |[begin, end)| is considered as offsets relative to |base_|.
// We chop it into segments using |abs32_locations_|, each with
// width |abs32_width_|. For example, given
//   [begin, end) = [base_ + 8, base_ + 25),
//   abs32_locations_ = {2, 6, 15, 20, 27},
//   abs32_width_ = 4,
// we obtain the following:
//             111111111122222222223   -> offsets
//   0123456789012345678901234567890
//   ........*****************......   -> main range = *
//     ^   ^        ^    ^      ^      -> abs32 locations
//     aaaaaaaa     aaaa aaaa   aaaa   -> abs32 locations with width
//   ........--*****----*----*......   -> main range got chopped into 3 pieces
// The resulting non-empty segments are:
//   [10, 15), [19, 20), [24, 25).
// These are then dispatched to SearchSegment() for rel32 detection.
void Rel32Finder::Search(Region::const_iterator begin,
                         Region::const_iterator end,
                         Rel32Finder::SearchCallback* callback) {
  const offset_t begin_offset = begin - base_;
  assert(abs32_width_ > 0);

  // Find the first |abs32_cur| with |*abs32_cur >= begin_offset|.
  std::vector<offset_t>::const_iterator abs32_cur = std::lower_bound(
      abs32_locations_.begin(), abs32_locations_.end(), begin_offset);

  // Find lower boundary, accounting for possibility that |*(abs32_cur - 1)|
  // may straddle across |begin|.
  Region::const_iterator lo = begin;
  if (abs32_cur > abs32_locations_.begin())
    lo = std::max(lo, base_ + *(abs32_cur - 1) + abs32_width_);

  // Iterate over |[abs32_cur, abs32_locations_.end())| and dispatch segments.
  while (abs32_cur < abs32_locations_.end() && base_ + *abs32_cur < end) {
    Region::const_iterator hi = base_ + *abs32_cur;
    if (lo < hi)
      SearchSegment(lo, hi, callback);
    lo = hi + abs32_width_;
    ++abs32_cur;
  }

  // Dispatch final segment.
  if (lo < end)
    SearchSegment(lo, end, callback);
}

/******** Rel32FinderX86 ********/

void Rel32FinderX86::SearchSegment(Region::const_iterator seg_begin,
                                   Region::const_iterator seg_end,
                                   Rel32Finder::SearchCallback* callback) {
  Region::const_iterator cur = seg_begin;
  while (cur < seg_end) {
    // Heuristic rel32 detection by looking for opcodes that use them.
    if (cur + 5 <= seg_end) {
      if (cur[0] == 0xE8 || cur[0] == 0xE9) {  // JMP rel32; CALL rel32
        rel32_ = {cur + 1, false};
        if ((*callback)()) {
          cur += 5;
          continue;
        }
      }
    }
    if (cur + 6 <= seg_end) {
      if (cur[0] == 0x0F && (cur[1] & 0xF0) == 0x80) {  // Jcc long form
        rel32_ = {cur + 2, false};
        if ((*callback)()) {
          cur += 6;
          continue;
        }
      }
    }
    ++cur;
  }
  rel32_ = {seg_end, false};
}

/******** Rel32FinderX64 ********/

void Rel32FinderX64::SearchSegment(Region::const_iterator seg_begin,
                                   Region::const_iterator seg_end,
                                   Rel32Finder::SearchCallback* callback) {
  Region::const_iterator cur = seg_begin;
  while (cur < seg_end) {
    // Heuristic rel32 detection by looking for opcodes that use them.
    if (cur + 5 <= seg_end) {
      if (cur[0] == 0xE8 || cur[0] == 0xE9) {  // JMP rel32; CALL rel32
        rel32_ = {cur + 1, false};
        if ((*callback)()) {
          cur += 5;
          continue;
        }
      }
    }
    if (cur + 6 <= seg_end) {
      if (cur[0] == 0x0F && (cur[1] & 0xF0) == 0x80) {  // Jcc long form
        rel32_ = {cur + 2, false};
        if ((*callback)()) {
          cur += 6;
          continue;
        }
      } else if ((cur[0] == 0xFF && (cur[1] == 0x15 || cur[1] == 0x25)) ||
                 ((cur[0] == 0x89 || cur[0] == 0x8B || cur[0] == 0x8D) &&
                  (cur[1] & 0xC7) == 0x05)) {
        // 6-byte instructions:
        // [2-byte opcode] [disp32]:
        //  Opcode
        //   FF 15: CALL QWORD PTR [rip+disp32]
        //   FF 25: JMP  QWORD PTR [rip+disp32]
        //
        // [1-byte opcode] [ModR/M] [disp32]:
        //  Opcode
        //   89: MOV DWORD PTR [rip+disp32],reg
        //   8B: MOV reg,DWORD PTR [rip+disp32]
        //   8D: LEA reg,[rip+disp32]
        //  ModR/M : MMRRRMMM
        //   MM = 00 & MMM = 101 => rip+disp32
        //   RRR: selects reg operand from [eax|ecx|...|edi]
        rel32_ = {cur + 2, true};
        if ((*callback)()) {
          cur += 6;
          continue;
        }
      }
    }
    ++cur;
  }
  rel32_ = {seg_end, false};
}

/******** Rel32FinderARM32 ********/

Rel32FinderARM32::Rel32FinderARM32(Region::const_iterator base,
                                   const std::vector<offset_t>& abs32_locations,
                                   int abs32_width,
                                   const AddressTranslator& trans)
    : Rel32Finder(base, abs32_locations, abs32_width),
      offset_to_rva_(trans.GetOffsetToRVATranslator()) {}

Rel32FinderARM32::~Rel32FinderARM32() = default;

void Rel32FinderARM32::SearchSegment(Region::const_iterator seg_begin,
                                     Region::const_iterator seg_end,
                                     Rel32Finder::SearchCallback* callback) {
  if (is_thumb2_)
    SearchSegmentT32(seg_begin, seg_end, callback);
  else
    SearchSegmentA32(seg_begin, seg_end, callback);
}

void Rel32FinderARM32::SearchSegmentA32(Region::const_iterator seg_begin,
                                        Region::const_iterator seg_end,
                                        Rel32Finder::SearchCallback* callback) {
  Region::const_iterator cur = seg_begin;
  // 4-byte alignment.
  if ((cur - base_) % 4)
    cur += 4 - (cur - base_) % 4;

  for (; seg_end - cur >= 4; cur += 4) {
    offset_t offset = static_cast<offset_t>(cur - base_);
    ARM32Rel32Parser parser(offset_to_rva_(offset));
    uint32_t code32 = parser.FetchARMCode32(cur);
    arm_disp_t disp = 0;

    if (parser.DecodeA24(code32, &disp)) {
      rel32_ = {offset, parser.GetARMTargetRVAFromDisplacement(disp),
                ARM32Rel32Parser::ADDR_A24};
      (*callback)();
    }
  }
  rel32_ = {static_cast<offset_t>(seg_end - base_),
            static_cast<offset_t>(seg_end - base_),
            ARM32Rel32Parser::ADDR_NONE};
}

void Rel32FinderARM32::SearchSegmentT32(Region::const_iterator seg_begin,
                                        Region::const_iterator seg_end,
                                        Rel32Finder::SearchCallback* callback) {
  Region::const_iterator cur = seg_begin;
  // 2-byte alignment.
  if ((cur - base_) % 2)
    cur += (cur - base_) % 2;

  Region::const_iterator next = seg_end;
  for (; seg_end - cur >= 2; cur = next) {
    offset_t offset = static_cast<offset_t>(cur - base_);
    ARM32Rel32Parser parser(offset_to_rva_(offset));
    uint16_t code16 = parser.FetchTHUMB2Code16(cur);
    int instr_size = GetTHUMB2InstructionSize(code16);
    next = cur + instr_size;
    arm_disp_t disp = 0;
    ARM32Rel32Parser::AddrType type = ARM32Rel32Parser::ADDR_NONE;

    if (instr_size == 2) {  // 16-bit THUMB2 instruction.
      if (parser.DecodeT8(code16, &disp))
        type = ARM32Rel32Parser::ADDR_T8;
      else if (parser.DecodeT11(code16, &disp))
        type = ARM32Rel32Parser::ADDR_T11;

    } else {  // |instr_size == 4|: 32-bit THUMB2 instruction.
      if (seg_end - cur >= 4) {
        uint32_t code32 = parser.FetchTHUMB2Code32(cur);
        if (parser.DecodeT21(code32, &disp))
          type = ARM32Rel32Parser::ADDR_T21;
        else if (parser.DecodeT24(code32, &disp))
          type = ARM32Rel32Parser::ADDR_T24;
      }
    }
    if (type != ARM32Rel32Parser::ADDR_NONE) {
      rel32_ = {offset, parser.GetTHUMB2TargetRVAFromDisplacement(disp), type};
      (*callback)();
    }
  }
  rel32_ = {static_cast<offset_t>(seg_end - base_),
            static_cast<offset_t>(seg_end - base_),
            ARM32Rel32Parser::ADDR_NONE};
}

/******** Rel32FinderAArch64 ********/

Rel32FinderAArch64::Rel32FinderAArch64(
    Region::const_iterator base,
    const std::vector<offset_t>& abs32_locations,
    const AddressTranslator& trans)
    : Rel32Finder(base, abs32_locations, 4),
      offset_to_rva_(trans.GetOffsetToRVATranslator()) {}

Rel32FinderAArch64::~Rel32FinderAArch64() = default;

void Rel32FinderAArch64::SearchSegment(Region::const_iterator seg_begin,
                                       Region::const_iterator seg_end,
                                       Rel32Finder::SearchCallback* callback) {
  Region::const_iterator cur = seg_begin;
  // 4-byte alignment.
  if ((cur - base_) % 4)
    cur += 4 - (cur - base_) % 4;

  for (; seg_end - cur >= 4; cur += 4) {
    offset_t offset = static_cast<offset_t>(cur - base_);
    // For simplicity we assume RVA fits within 32-bits.
    AArch64Rel32Parser parser(offset_to_rva_(offset));
    uint32_t code32 = parser.Fetch(cur);
    arm_disp_t disp = 0;
    AArch64Rel32Parser::AddrType type = AArch64Rel32Parser::ADDR_NONE;

    if (parser.DecodeImmd14(code32, &disp)) {
      type = AArch64Rel32Parser::ADDR_IMMD14;
    } else if (parser.DecodeImmd19(code32, &disp)) {
      type = AArch64Rel32Parser::ADDR_IMMD19;
    } else if (parser.DecodeImmd26(code32, &disp)) {
      type = AArch64Rel32Parser::ADDR_IMMD26;
    }
    if (type != AArch64Rel32Parser::ADDR_NONE) {
      rel32_ = {offset, parser.GetTargetRVAFromDisplacement(disp), type};
      (*callback)();
    }
  }
  rel32_ = {static_cast<offset_t>(seg_end - base_),
            static_cast<offset_t>(seg_end - base_),
            AArch64Rel32Parser::ADDR_NONE};
}

}  // namespace zucchini
