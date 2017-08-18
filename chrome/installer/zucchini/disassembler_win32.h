// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_DISASSEMBLER_WIN32_H_
#define CHROME_INSTALLER_ZUCCHINI_DISASSEMBLER_WIN32_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/macros.h"
#include "chrome/installer/zucchini/address_translator.h"
#include "chrome/installer/zucchini/buffer_view.h"
#include "chrome/installer/zucchini/disassembler.h"
#include "chrome/installer/zucchini/image_utils.h"
#include "chrome/installer/zucchini/rel32_finder.h"
#include "chrome/installer/zucchini/type_win_pe.h"

namespace zucchini {

struct Win32X86Traits {
  static constexpr ExecutableType kExeType = kExeTypeWin32X86;
  static const char kExeTypeString[];
  static constexpr uint16_t kMagic = 0x10B;
  static constexpr uint16_t kRelocType = 3;
  static constexpr offset_t kVAWidth = 4;

  using ImageOptionalHeader = pe::ImageOptionalHeader;
  using RelFinder = Rel32FinderX86;
  using Address = uint32_t;
};

struct Win32X64Traits {
  static constexpr ExecutableType kExeType = kExeTypeWin32X64;
  static const char kExeTypeString[];
  static constexpr uint16_t kMagic = 0x20B;
  static constexpr uint16_t kRelocType = 10;
  static constexpr offset_t kVAWidth = 8;

  using ImageOptionalHeader = pe::ImageOptionalHeader64;
  using RelFinder = Rel32FinderX64;
  using Address = uint64_t;
};

template <class Traits>
class DisassemblerWin32 : public Disassembler, public AddressTranslatorFactory {
 public:
  enum ReferenceType : uint8_t { kRel32, kTypeCount };

  // Applies quick checks to determine whether |image| *may* point to the start
  // of an executable. Returns true iff the check passes.
  static bool QuickDetect(ConstBufferView image);

  // Main instantiator and initializer.
  static std::unique_ptr<DisassemblerWin32> Make(ConstBufferView image);

  DisassemblerWin32();
  ~DisassemblerWin32() override;

  // Disassembler:
  ExecutableType GetExeType() const override;
  std::string GetExeTypeString() const override;
  std::vector<ReferenceGroup> MakeReferenceGroups() const override;

  // AddressTranslator:
  std::unique_ptr<RVAToOffsetTranslator> MakeRVAToOffsetTranslator()
      const override;
  std::unique_ptr<OffsetToRVATranslator> MakeOffsetToRVATranslator()
      const override;

  // Functions that returns iterators to read / write references.
  std::unique_ptr<ReferenceReader> ReadRel32(offset_t lower, offset_t upper);
  std::unique_ptr<ReferenceWriter> WriteRel32(MutableBufferView image);

 private:
  // Helpers for PE parsing.
  static const pe::ImageDataDirectory* ReadDataDirectory(
      const typename Traits::ImageOptionalHeader* optional_header,
      size_t index);
  static bool IsCodeSection(const pe::ImageSectionHeader& section);

  // Disassembler:
  bool Parse(ConstBufferView image) override;

  // Parses the file header. Returns true iff successful.
  bool ParseHeader();

  // Parsers to extract references. These are lazily called, and return whether
  // parsing was successful (failures are non-fatal).
  bool ParseAndStoreRel32();

  // Translation utilities.
  const pe::ImageSectionHeader* RVAToSection(rva_t rva) const;
  const pe::ImageSectionHeader* OffsetToSection(offset_t offset) const;
  offset_t RVAToOffset(rva_t rva) const;
  rva_t OffsetToRVA(offset_t offset) const;
  rva_t AddressToRVA(typename Traits::Address address) const;
  typename Traits::Address RVAToAddress(rva_t rva) const;

  // Internal PE file states.
  uint16_t sections_count_ = 0;
  const pe::ImageSectionHeader* sections_ = nullptr;
  typename Traits::Address image_base_ = 0;
  // Size of image once loaded in memory, not to be confused with stored size.
  uint32_t size_of_image_ = 0;

  // Maps for address translation. The second element of each std::pair is an
  // index in [0, sections_count_) into |sections_|.
  std::vector<std::pair<rva_t, int>> section_rva_map_;
  std::vector<std::pair<offset_t, int>> section_offset_map_;

  // Reference storage.
  std::vector<offset_t> rel32_locations_;

  // Initialization states of reference stroage, used for lazy initialization.
  bool has_parsed_rel32_ = false;

  DISALLOW_COPY_AND_ASSIGN(DisassemblerWin32);
};

using DisassemblerWin32X86 = DisassemblerWin32<Win32X86Traits>;
using DisassemblerWin32X64 = DisassemblerWin32<Win32X64Traits>;

}  // namespace zucchini

#endif  // CHROME_INSTALLER_ZUCCHINI_DISASSEMBLER_WIN32_H_
