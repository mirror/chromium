// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/rel32_finder.h"

namespace zucchini {

/******** Abs32CollisionDetector ********/

bool Abs32CollisionDetector::Collides(offset_t offset, size_t length) {
  // Advance |abs32_current_| while it lies entirely before |*abs_current_|.
  while (abs32_current_ != abs32_end_ &&
         *abs32_current_ + abs32_width_ <= offset)
    ++abs32_current_;

  DCHECK(abs32_current_ == abs32_end_ ||
         *abs32_current_ + abs32_width_ > offset);

  // |abs32_current_| (aaaa) has caught up. Collision with
  // [|offset|, |offset| + |length|) (oooo) can occur by tail and/or by head,
  // e.g.:
  //   aaaa...      aaaa      ...aaaa
  //   ...oooo      oooo      oooo...
  // It suffices to check whether head of "aaaa" clears the tail of "oooo".
  return abs32_current_ != abs32_end_ && *abs32_current_ < offset + length;
}

ConstBufferView::const_iterator Rel32FinderX86::Scan() {
  while (cursor_ < end_) {
    // Heuristic rel32 detection by looking for opcodes that use them.
    if (cursor_ + 5 <= end_) {
      if (cursor_[0] == 0xE8 || cursor_[0] == 0xE9) {  // JMP rel32; CALL rel32
        rel32_ = {cursor_ + 1, false};
        return rel32_.location + 4;
      }
    }
    if (cursor_ + 6 <= end_) {
      if (cursor_[0] == 0x0F && (cursor_[1] & 0xF0) == 0x80) {  // Jcc long form
        rel32_ = {cursor_ + 2, false};
        return rel32_.location + 4;
      }
    }
    ++cursor_;
  }
  return end_;
}

/******** Rel32FinderX64 ********/

ConstBufferView::const_iterator Rel32FinderX64::Scan() {
  while (cursor_ < end_) {
    // Heuristic rel32 detection by looking for opcodes that use them.
    if (cursor_ + 5 <= end_) {
      if (cursor_[0] == 0xE8 || cursor_[0] == 0xE9) {  // JMP rel32; CALL rel32
        rel32_ = {cursor_ + 1, false};
        return rel32_.location + 4;
      }
    }
    if (cursor_ + 6 <= end_) {
      if (cursor_[0] == 0x0F && (cursor_[1] & 0xF0) == 0x80) {  // Jcc long form
        rel32_ = {cursor_ + 2, false};
        return rel32_.location + 4;
      } else if ((cursor_[0] == 0xFF &&
                  (cursor_[1] == 0x15 || cursor_[1] == 0x25)) ||
                 ((cursor_[0] == 0x89 || cursor_[0] == 0x8B ||
                   cursor_[0] == 0x8D) &&
                  (cursor_[1] & 0xC7) == 0x05)) {
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
        rel32_ = {cursor_ + 2, true};
        return rel32_.location + 4;
      }
    }
    ++cursor_;
  }
  return end_;
}

}  // namespace zucchini
