// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/reference_bytes_mixer.h"

#include <algorithm>
#include <iostream>

#include "base/logging.h"
#include "chrome/installer/zucchini/disassembler_elf.h"
#include "chrome/installer/zucchini/io_utils.h"

namespace zucchini {

/******** ReferenceBytesMixer ********/

// Default implementation is a stub, i.e., for architectures whose rel32
// instructions have operation bits and payload bits stored in separate bytes.
// So during patch application, payload bits are copied for matched blocks,
// ignored by bytewise corrections, and fixed by reference target corrections.
ReferenceBytesMixer::ReferenceBytesMixer() {}

ReferenceBytesMixer::~ReferenceBytesMixer() = default;

// static.
std::unique_ptr<ReferenceBytesMixer> ReferenceBytesMixer::Create(
    const Disassembler& src_dis,
    const Disassembler& dst_dis) {
  ExecutableType exe_type = src_dis.GetExeType();
  DCHECK_EQ(exe_type, dst_dis.GetExeType());
  if (exe_type == kExeTypeElfArm32) {
    const DisassemblerElfARM32& src_arm_dis =
        static_cast<const DisassemblerElfARM32&>(src_dis);
    const DisassemblerElfARM32& dst_arm_dis =
        static_cast<const DisassemblerElfARM32&>(dst_dis);
    return std::unique_ptr<ReferenceBytesMixer>(new ReferenceBytesMixerElfARM(
        exe_type, src_arm_dis.translator(), dst_arm_dis.translator()));
  }
  if (exe_type == kExeTypeElfAArch64) {
    const DisassemblerElfAArch64& src_arm_dis =
        static_cast<const DisassemblerElfAArch64&>(src_dis);
    const DisassemblerElfAArch64& dst_arm_dis =
        static_cast<const DisassemblerElfAArch64&>(dst_dis);
    return std::unique_ptr<ReferenceBytesMixer>(new ReferenceBytesMixerElfARM(
        exe_type, src_arm_dis.translator(), dst_arm_dis.translator()));
  }
  return std::unique_ptr<ReferenceBytesMixer>(new ReferenceBytesMixer);
}

// Stub implementation.
int ReferenceBytesMixer::NumBytes(uint8_t type) const {
  return 0;
}

// Stub implementation.
void ReferenceBytesMixer::Mix(uint8_t type,
                              ConstBufferView::const_iterator old_base,
                              offset_t old_offset,
                              ConstBufferView::const_iterator new_base,
                              offset_t new_offset,
                              MutableBufferView::iterator out) const {}

/******** ReferenceBytesMixerElfARM ********/

ReferenceBytesMixerElfARM::ReferenceBytesMixerElfARM(
    ExecutableType exe_type,
    const AddressTranslator& old_translator,
    const AddressTranslator& new_translator)
    : exe_type_(exe_type),
      old_offset_to_rva_(old_translator),
      new_offset_to_rva_(new_translator) {}

ReferenceBytesMixerElfARM::~ReferenceBytesMixerElfARM() = default;

int ReferenceBytesMixerElfARM::NumBytes(uint8_t type) const {
  if (exe_type_ == kExeTypeElfArm32) {
    switch (type) {
      case ARM32ReferenceType::kRel32_A24:  // Falls through.
      case ARM32ReferenceType::kRel32_T21:
      case ARM32ReferenceType::kRel32_T24:
        return 4;
      case ARM32ReferenceType::kRel32_T8:  // Falls through.
      case ARM32ReferenceType::kRel32_T11:
        return 2;
    }
  } else if (exe_type_ == kExeTypeElfAArch64) {
    switch (type) {
      case AArch64ReferenceType::kRel32_Immd14:  // Falls through.
      case AArch64ReferenceType::kRel32_Immd19:
      case AArch64ReferenceType::kRel32_Immd26:
        return 4;
    }
  }
  return 0;
}

void ReferenceBytesMixerElfARM::Mix(uint8_t type,
                                    ConstBufferView::const_iterator old_base,
                                    offset_t old_offset,
                                    ConstBufferView::const_iterator new_base,
                                    offset_t new_offset,
                                    MutableBufferView::iterator out) const {
  int num_bytes = NumBytes(type);
  ConstBufferView::const_iterator old_it = old_base + old_offset;
  ConstBufferView::const_iterator new_it = new_base + new_offset;
  std::copy(new_it, new_it + num_bytes, out);

  ARMCopyDisplacementFun copier = GetCopier(type);
  DCHECK_NE(copier, nullptr);

  if (!copier(old_offset_to_rva_.Convert(old_offset), old_it,
              new_offset_to_rva_.Convert(new_offset), out)) {
    // Failure: A potential cause is THUMB2 code misidentification. This is not
    // fatal, so print warning.
    static LimitedOutputStream los(std::cerr, 10);
    if (!los.full()) {
      los << "Warning: Reference byte mix failed with type = "
          << static_cast<uint32_t>(type) << "." << std::endl;
    }
    // Fall back to direct copy.
    std::copy(new_it, new_it + num_bytes, out);
  }
}

ARMCopyDisplacementFun ReferenceBytesMixerElfARM::GetCopier(
    uint8_t type) const {
  if (exe_type_ == kExeTypeElfArm32) {
    switch (type) {
      case ARM32ReferenceType::kRel32_A24:
        return ARMCopyDisplacement<ARM32Rel32Parser::AddrTraits_A24>;
      case ARM32ReferenceType::kRel32_T8:
        return ARMCopyDisplacement<ARM32Rel32Parser::AddrTraits_T8>;
      case ARM32ReferenceType::kRel32_T11:
        return ARMCopyDisplacement<ARM32Rel32Parser::AddrTraits_T11>;
      case ARM32ReferenceType::kRel32_T21:
        return ARMCopyDisplacement<ARM32Rel32Parser::AddrTraits_T21>;
      case ARM32ReferenceType::kRel32_T24:
        return ARMCopyDisplacement<ARM32Rel32Parser::AddrTraits_T24>;
    }
  } else if (exe_type_ == kExeTypeElfAArch64) {
    switch (type) {
      case AArch64ReferenceType::kRel32_Immd14:
        return ARMCopyDisplacement<AArch64Rel32Parser::AddrTraits_Immd14>;
      case AArch64ReferenceType::kRel32_Immd19:
        return ARMCopyDisplacement<AArch64Rel32Parser::AddrTraits_Immd19>;
      case AArch64ReferenceType::kRel32_Immd26:
        return ARMCopyDisplacement<AArch64Rel32Parser::AddrTraits_Immd26>;
    }
  }
  DLOG(FATAL) << "NOTREACHED";
  return nullptr;
}

}  // namespace zucchini
