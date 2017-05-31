// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZUCCHINI_DISASSEMBLER_WIN32_H_
#define ZUCCHINI_DISASSEMBLER_WIN32_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/macros.h"
#include "zucchini/disassembler.h"
#include "zucchini/image_utils.h"
#include "zucchini/region.h"
#include "zucchini/rel32_finder.h"
#include "zucchini/rel32_utils.h"
#include "zucchini/type_win_pe.h"

namespace zucchini {

struct Win32X86Traits {
  static constexpr ExecutableType kExeType = EXE_TYPE_WIN32_X86;
  static const char kExeTypeString[];
  static constexpr uint16_t kMagic = 0x10b;
  static constexpr uint16_t kRelocType = 3;
  static constexpr offset_t kVAWidth = 4;

  using ImageOptionalHeader = pe::ImageOptionalHeader;
  using RelFinder = Rel32FinderX86;
  using Address = uint32_t;
};

struct Win32X64Traits {
  static constexpr ExecutableType kExeType = EXE_TYPE_WIN32_X64;
  static const char kExeTypeString[];
  static constexpr uint16_t kMagic = 0x20b;
  static constexpr uint16_t kRelocType = 10;
  static constexpr offset_t kVAWidth = 8;

  using ImageOptionalHeader = pe::ImageOptionalHeader64;
  using RelFinder = Rel32FinderX64;
  using Address = uint64_t;
};

template <class Traits>
class DisassemblerWin32 : public Disassembler {
 public:
  enum ReferenceType : uint8_t { kReloc, kAbs32, kRel32, kTypeCount };
  static const ReferenceTraits kGroups[];

  explicit DisassemblerWin32(Region image);

  // Applies quick checks to determine if |image| *may* point to the start of an
  // executable. Returns true on success.
  static bool QuickDetect(Region image);

  // Disassembler:
  ExecutableType GetExeType() const override;
  std::string GetExeTypeString() const override;
  ReferenceTraits GetReferenceTraits(uint8_t type) const override;
  uint8_t GetReferenceTraitsCount() const override;
  bool Parse() override;

  // AddressTranslator:
  RVAToOffsetTranslator GetRVAToOffsetTranslator() const override;
  OffsetToRVATranslator GetOffsetToRVATranslator() const override;

  ReferenceGenerator FindRelocs(offset_t lower, offset_t upper);
  ReferenceGenerator FindAbs32(offset_t lower, offset_t upper);
  ReferenceGenerator FindRel32(offset_t lower, offset_t upper);
  ReferenceReceptor ReceiveRelocs();
  ReferenceReceptor ReceiveAbs32();
  ReferenceReceptor ReceiveRel32() { return Rel32ReceptorX86(this, image_); }

 private:
  static const pe::ImageDataDirectory* ReadDataDirectory(
      const typename Traits::ImageOptionalHeader* optional_header,
      size_t index);
  static bool IsCodeSection(const pe::ImageSectionHeader* section);

  bool ParseHeader();
  bool ParseRelocs();
  bool GetAbs32FromRelocs();
  bool ParseSections();
  bool ParseSection(const pe::ImageSectionHeader& section);

  const pe::ImageSectionHeader* RVAToSection(rva_t rva) const;
  const pe::ImageSectionHeader* OffsetToSection(offset_t offset) const;
  offset_t RVAToOffset(rva_t rva) const;
  rva_t OffsetToRVA(offset_t offset) const;
  rva_t AddressToRVA(typename Traits::Address address) const;
  typename Traits::Address RVAToAddress(rva_t rva) const;

  const pe::ImageSectionHeader* sections_ = nullptr;
  const pe::ImageDataDirectory* base_relocation_table_ = nullptr;
  uint16_t sections_count_ = 0;

  // Range limited to 32 bits for 32-bit executable
  typename Traits::Address image_base_ = 0;
  // Size of image once loaded in memory, not to be confused with stored size.
  uint32_t size_of_image_ = 0;

  std::vector<std::pair<rva_t, int>> section_rva_map_;
  std::vector<std::pair<offset_t, int>> section_offset_map_;

  std::vector<offset_t> abs32_locations_;
  std::vector<offset_t> reloc_blocks_;
  std::vector<offset_t> rel32_locations_;

  DISALLOW_COPY_AND_ASSIGN(DisassemblerWin32);
};

using DisassemblerWin32X86 = DisassemblerWin32<Win32X86Traits>;
using DisassemblerWin32X64 = DisassemblerWin32<Win32X64Traits>;

}  // namespace zucchini

#endif  // ZUCCHINI_DISASSEMBLER_WIN32_H_
