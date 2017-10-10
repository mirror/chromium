// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_REFERENCE_BYTES_MIXER_H_
#define CHROME_INSTALLER_ZUCCHINI_REFERENCE_BYTES_MIXER_H_

#include <stdint.h>

#include <memory>

#include "base/macros.h"
#include "chrome/installer/zucchini/buffer_view.h"
#include "chrome/installer/zucchini/disassembler.h"
#include "chrome/installer/zucchini/image_utils.h"
#include "chrome/installer/zucchini/rel32_utils.h"

namespace zucchini {

class Disassembler;

// Rel32 references are stored in architecture-dependent formats. This affects
// bytewise diff generation for matched blocks. In general, bits in a rel32
// instruction fall under 2 categories:
// - Operation bits: Instruction op code and conditionals.
// - Payload bits: Target data, typically as displacements relative to
//   instruction pointer / program counter.
// Patch application transforms
//   Old rel32 instruction = {old operation, old payload},
// to
//   New rel32 instruction = {new operation, new payload}.
// During patch application, new image bytes are written by three sources:
//   (1) Direct copy from old image to new image for matched blocks.
//   (2) Bytewise diff correction.
//   (3) Dedicated reference (in this case, rel32) target correction.
//
// For architectures where rel32 operation and payload bits are stored in
// separate bytes (e.g., X86), (2) can ignore payload bits (bytes). So during
// patch application, (1) naively copies rel32 instructions, (2) fixes
// operation bytes only, and (3) fixes payload bytes.
//
// For architectures where rel32 operation and payload bits can share common
// bytes (e.g., ARM), we have a dilemma:
// - If we ignore these in (2), then we'd fail to copy new operation bits.
// - If we use (2) for these bytes, then we'd negate the benefits of reference
//   extraction.
//
// Solution: We use a hybrid approach: For each matching old / new rel32
// instruction pair, define:
//   Mixed rel32 instruction = {new operation, old payload},
//
// During patch generation, we compute bytewise correction from old rel32
// instruction to the mixed rel32 instruction. So during patch application, (2)
// only corrects operation bit changes, and (3) overwrites old payload bits to
// new payload bits.

// An interface for mixed rel32 instructions generation. This is only needed for
// architectures whose instructions store operation bits and payload bits in
// common bytes (e.g., ARM).
class ReferenceBytesMixer {
 public:
  ReferenceBytesMixer();
  virtual ~ReferenceBytesMixer();

  // Returns a new ReferenceBytesMixer instance that's owned by the caller.
  static std::unique_ptr<ReferenceBytesMixer> Create(
      const Disassembler& src_dis,
      const Disassembler& dst_dis);

  // Returns the number of bytes that need to be mixed, for references with
  // given |type|. Returns 0 if no mixing is required (e.g., for abs32).
  virtual int NumBytes(uint8_t type) const;

  // Given |old_base[old_offset]| and |new_base[new_offset]|, each pointing to a
  // rel32 instruction with |type|, computes the mixed rel32 instruction that
  // combines payload bits from the old instruction with operation bits from the
  // new instruction, and writes the result to preallocated buffer |*out|, which
  // is assumed to be distinct from inputs.
  virtual void Mix(uint8_t type,
                   ConstBufferView::const_iterator old_base,
                   offset_t old_offset,
                   ConstBufferView::const_iterator new_base,
                   offset_t new_offset,
                   MutableBufferView::iterator out) const;

 private:
  DISALLOW_COPY_AND_ASSIGN(ReferenceBytesMixer);
};

// In ARM32 and AArch64, instructions mix operation bits and payload bits in
// complex ways. This is the main use case of ReferenceBytesMixer.
class ReferenceBytesMixerElfARM : public ReferenceBytesMixer {
 public:
  // |exe_type| must be EXE_TYPE_ELF_ARM or EXE_TYPE_ELF_AARCH64.
  ReferenceBytesMixerElfARM(ExecutableType exe_type,
                            const AddressTranslator& old_translator,
                            const AddressTranslator& new_translator);
  ~ReferenceBytesMixerElfARM() override;

  // ReferenceBytesMixer:
  int NumBytes(uint8_t type) const override;
  void Mix(uint8_t type,
           ConstBufferView::const_iterator old_base,
           offset_t old_offset,
           ConstBufferView::const_iterator new_base,
           offset_t new_offset,
           MutableBufferView::iterator out) const override;

 private:
  ARMCopyDisplacementFun GetCopier(uint8_t type) const;

  // For simplicity, 32-bit vs. 64-bit distinction is represented by state
  // |exe_type_|, rather than having inherited classes.
  const ExecutableType exe_type_;
  mutable AddressTranslator::OffsetToRvaCache old_offset_to_rva_;
  mutable AddressTranslator::OffsetToRvaCache new_offset_to_rva_;

  DISALLOW_COPY_AND_ASSIGN(ReferenceBytesMixerElfARM);
};

}  // namespace zucchini

#endif  // CHROME_INSTALLER_ZUCCHINI_REFERENCE_BYTES_MIXER_H_
