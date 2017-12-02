// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_RELOC_UTILS_H_
#define CHROME_INSTALLER_ZUCCHINI_RELOC_UTILS_H_

#include <stddef.h>
#include <stdint.h>

#include <vector>

#include "base/numerics/safe_conversions.h"
#include "base/optional.h"
#include "chrome/installer/zucchini/address_translator.h"
#include "chrome/installer/zucchini/buffer_source.h"
#include "chrome/installer/zucchini/buffer_view.h"
#include "chrome/installer/zucchini/image_utils.h"
#include "chrome/installer/zucchini/type_elf.h"

namespace zucchini {

// Win32 PE relocation table stores a list of (type, RVA) pairs. The table is
// organized into "blocks" for RVAs with common high-order bits (12-31). Each
// block consists of a list (even length) of 2-byte "units". Each unit stores
// type (in bits 12-15) and low-order bits (0-11) of an RVA (in bits 0-11). In
// pseudo-struct:
//   struct Block {
//     uint32_t rva_hi;
//     uint32_t block_size_in_bytes;  // 8 + multiple of 4.
//     struct {
//       uint16_t rva_lo:12, type:4;  // Little-endian.
//     } units[(block_size_in_bytes - 8) / 2];  // Size must be even.
//   } reloc_table[num_blocks];  // May have padding (type = 0).

// Extracted Win32 reloc Unit data.
struct RelocUnitWin32 {
  RelocUnitWin32();
  RelocUnitWin32(uint8_t type_in, offset_t location_in, rva_t target_rva_in);
  friend bool operator==(const RelocUnitWin32& a, const RelocUnitWin32& b);

  uint8_t type;
  offset_t location;
  rva_t target_rva;
};

// A reader that parses Win32 PE relocation data and emits RelocUnitWin32 for
// each reloc unit that lies strictly inside |[lo, hi)|.
class RelocRvaReaderWin32 {
 public:
  enum : ptrdiff_t { kRelocUnitSize = sizeof(uint16_t) };

  // Parses |image| at |reloc_region| to find beginning offsets of each reloc
  // block. On success, writes the result to |reloc_block_offsets| and returns
  // true. Otherwise leaves |reloc_block_offsets| in an undetermined state, and
  // returns false.
  static bool FindRelocBlocks(ConstBufferView image,
                              BufferRegion reloc_region,
                              std::vector<offset_t>* reloc_block_offsets);

  // |reloc_block_offsets| should be precomputed from FindRelBlocks().
  RelocRvaReaderWin32(ConstBufferView image,
                      BufferRegion reloc_region,
                      const std::vector<offset_t>& reloc_block_offsets,
                      offset_t lo,
                      offset_t hi);
  RelocRvaReaderWin32(RelocRvaReaderWin32&&);
  ~RelocRvaReaderWin32();

  // Successively visits and returns data for each reloc unit, or base::nullopt
  // when all reloc units are found. Encapsulates block transition details.
  base::Optional<RelocUnitWin32> GetNext();

 private:
  // Assuming that |block_begin| points to the beginning of a reloc block, loads
  // |rva_hi_bits_| and assigns |cur_reloc_units_| as the region containing the
  // associated units, potentially truncated by |end_it_|. Returns true if reloc
  // data are available for read, and false otherwise.
  bool LoadRelocBlock(ConstBufferView::const_iterator block_begin);

  const ConstBufferView image_;

  // End iterator.
  ConstBufferView::const_iterator end_it_;

  // Unit data of the current reloc block.
  BufferSource cur_reloc_units_;

  // High-order bits (12-31) for all relocs of the current reloc block.
  rva_t rva_hi_bits_;
};

// A reader for Win32 reloc References, implemented as a filtering and
// translation adaptor of RelocRvaReaderWin32.
class RelocReaderWin32 : public ReferenceReader {
 public:
  // Takes ownership of |reloc_rva_reader|. |offset_bound| specifies the
  // exclusive upper bound of reloc target offsets, taking account of widths of
  // targets (which are abs32 References).
  RelocReaderWin32(RelocRvaReaderWin32&& reloc_rva_reader,
                   uint16_t reloc_type,
                   offset_t offset_bound,
                   const AddressTranslator& translator);
  ~RelocReaderWin32() override;

  // ReferenceReader:
  base::Optional<Reference> GetNext() override;

 private:
  RelocRvaReaderWin32 reloc_rva_reader_;
  const uint16_t reloc_type_;  // uint16_t to simplify shifting (<< 12).
  const offset_t offset_bound_;
  AddressTranslator::RvaToOffsetCache entry_rva_to_offset_;
};

// A writer for Win32 reloc References. This is simpler than the reader since:
// - No iteration is required.
// - High-order bits of reloc target RVAs are assumed to be handled elsewhere,
//   so only low-order bits need to be written.
class RelocWriterWin32 : public ReferenceWriter {
 public:
  RelocWriterWin32(uint16_t reloc_type,
                   MutableBufferView image,
                   BufferRegion reloc_region,
                   const std::vector<offset_t>& reloc_block_offsets,
                   const AddressTranslator& translator);
  ~RelocWriterWin32() override;

  // ReferenceWriter:
  void PutNext(Reference ref) override;

 private:
  const uint16_t reloc_type_;
  MutableBufferView image_;
  BufferRegion reloc_region_;
  const std::vector<offset_t>& reloc_block_offsets_;
  AddressTranslator::OffsetToRvaCache target_offset_to_rva_;
};

// Section dimensions for ELF files, to store relevant dimensions data from
// Elf32_Shdr and Elf64_Shdr, while reducing code duplication from templates.
struct SectionDimsElf {
 public:
  SectionDimsElf() = default;

  template <class Elf_Shdr>
  explicit SectionDimsElf(Elf_Shdr* section)
      : region(BufferRegion{base::checked_cast<size_t>(section->sh_offset),
                            base::checked_cast<size_t>(section->sh_size)}),
        entsize(static_cast<offset_t>(section->sh_entsize)) {}

  friend bool operator<(const SectionDimsElf& a, const SectionDimsElf& b) {
    return a.region.offset < b.region.offset;
  }

  friend bool operator<(offset_t offset, const SectionDimsElf& section) {
    return offset < section.region.offset;
  }

  BufferRegion region;
  offset_t entsize;
};

// A Generator to visit all reloc structs located in [|lo|, |hi|) (excluding
// truncated strct at |lo| but inlcuding truncated struct at |hi|), and emit
// valid References with |rel_type|. This implements a nested loop unrolled into
// a generator: the outer loop has |cur_section_dims_| visiting
// |reloc_section_dims| (sorted by |region.offset|), and the inner loop has
// |cursor_| visiting successive reloc structs within |cur_section_dims_|.
class RelocReaderElf : public ReferenceReader {
 public:
  RelocReaderElf(ConstBufferView image,
                 Bitness bitness,
                 const std::vector<SectionDimsElf>& reloc_section_dims,
                 uint32_t rel_type,
                 offset_t lo,
                 offset_t hi,
                 const AddressTranslator& translator);
  ~RelocReaderElf() override;

  // If |rel| contains |r_offset| for |rel_type_|, return the RVA. Otherwise
  // return |kInvalidRva|. These also handle Elf*_Rela, by using the fact that
  // Elf*_Rel is a prefix of Elf*_Rela.
  rva_t RelocTarget(Elf32_Rel rel) const;
  rva_t RelocTarget(Elf64_Rel rel) const;

  // ReferenceReader:
  base::Optional<Reference> GetNext() override;

 private:
  ConstBufferView image_;
  const Bitness bitness_;
  const std::vector<SectionDimsElf>& reloc_section_dims_;
  uint32_t rel_type_;
  offset_t hi_;
  std::vector<SectionDimsElf>::const_iterator cur_section_dims_;
  offset_t cursor_;
  offset_t cur_section_dims_end_;
  int cur_entsize_;  // Varies across REL / RELA sections.
  AddressTranslator::RvaToOffsetCache target_rva_to_offset_;
};

class RelocWriterElf : public ReferenceWriter {
 public:
  RelocWriterElf(MutableBufferView image,
                 Bitness bitness,
                 const AddressTranslator& translator);
  ~RelocWriterElf() override;

  // ReferenceWriter:
  void PutNext(Reference ref) override;

 private:
  MutableBufferView image_;
  const Bitness bitness_;
  AddressTranslator::OffsetToRvaCache target_offset_to_rva_;
};

}  // namespace zucchini

#endif  // CHROME_INSTALLER_ZUCCHINI_RELOC_UTILS_H_
