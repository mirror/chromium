// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "zucchini/program_detector.h"

#include <cassert>
#include <utility>

#include "base/logging.h"
#include "zucchini/disassembler_dex.h"
#include "zucchini/disassembler_elf.h"
#include "zucchini/disassembler_no_op.h"
#include "zucchini/disassembler_win32.h"

namespace zucchini {

namespace {

// Impose a minimal program size to eliminate pathlogical cases.
constexpr size_t kMinProgramSize = 16;

}  // namespace

/******** Utility Functions ********/

std::unique_ptr<Disassembler> MakeDisassemblerWithoutFallback(Region image) {
  std::unique_ptr<Disassembler> disasm;

  if (DisassemblerWin32X86::QuickDetect(image)) {
    disasm.reset(new DisassemblerWin32X86(image));
    if (disasm->Parse() && disasm->size() >= kMinProgramSize)
      return disasm;
  }

  if (DisassemblerWin32X64::QuickDetect(image)) {
    disasm.reset(new DisassemblerWin32X64(image));
    if (disasm->Parse() && disasm->size() >= kMinProgramSize)
      return disasm;
  }

  if (DisassemblerElfX86::QuickDetect(image)) {
    disasm.reset(new DisassemblerElfX86(image));
    if (disasm->Parse() && disasm->size() >= kMinProgramSize)
      return disasm;
  }

  if (DisassemblerElfARM32::QuickDetect(image)) {
    disasm.reset(new DisassemblerElfARM32(image));
    if (disasm->Parse() && disasm->size() >= kMinProgramSize)
      return disasm;
  }

  if (DisassemblerElfAArch64::QuickDetect(image)) {
    disasm.reset(new DisassemblerElfAArch64(image));
    if (disasm->Parse() && disasm->size() >= kMinProgramSize)
      return disasm;
  }

  if (DisassemblerDex::QuickDetect(image)) {
    disasm.reset(new DisassemblerDex(image));
    if (disasm->Parse() && disasm->size() >= kMinProgramSize)
      return disasm;
  }

  return nullptr;
}

std::unique_ptr<Disassembler> MakeDisassembler(Region image) {
  std::unique_ptr<Disassembler> disasm = MakeDisassemblerWithoutFallback(image);
  if (!disasm) {
    disasm.reset(new DisassemblerNoOp(image));
    if (!disasm->Parse()) {
      DLOG(FATAL) << "NOTREACHED";
    }
  }
  return disasm;
}

std::unique_ptr<Disassembler> MakeNoOpDisassembler(Region image) {
  return std::unique_ptr<Disassembler>(new DisassemblerNoOp(image));
}

/******** DisassemblerSingleProgramDetector ********/

DisassemblerSingleProgramDetector::DisassemblerSingleProgramDetector() {}

DisassemblerSingleProgramDetector::~DisassemblerSingleProgramDetector() =
    default;

bool DisassemblerSingleProgramDetector::Run(Region sub_image) {
  dis_ = zucchini::MakeDisassemblerWithoutFallback(sub_image);
  if (!dis_)
    return false;
  exe_type_ = dis_->GetExeType();
  region_ = Region(sub_image.begin(), dis_->size());
  return true;
}

/******** ProgramDetector ********/

ProgramDetector::ProgramDetector(Region image) : image_(image) {}

ProgramDetector::~ProgramDetector() = default;

bool ProgramDetector::GetNext(SingleProgramDetector* detector) {
  assert(detector);
  while (pos_ < image_.size()) {
    Region test_image(image_.begin() + pos_, image_.size() - pos_);
    if (detector->Run(test_image)) {
      pos_ += detector->region().size();
      return true;
    } else {
      ++pos_;
    }
  }
  return false;
}

}  // namespace zucchini
