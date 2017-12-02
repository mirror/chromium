// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_DISASSEMBLER_ELF_H_
#define CHROME_INSTALLER_ZUCCHINI_DISASSEMBLER_ELF_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "chrome/installer/zucchini/address_translator.h"
#include "chrome/installer/zucchini/buffer_view.h"
#include "chrome/installer/zucchini/disassembler.h"
#include "chrome/installer/zucchini/image_utils.h"
#include "chrome/installer/zucchini/rel32_finder.h"
#include "chrome/installer/zucchini/rel32_utils.h"
#include "chrome/installer/zucchini/reloc_utils.h"
#include "chrome/installer/zucchini/type_elf.h"

namespace zucchini {

struct ARMReferencePool {
  enum : uint8_t {
    kPoolReloc,
    kPoolAbs32,
    kPoolRel32,
  };
};

struct ARM32ReferenceType {
  enum : uint8_t {
    kReloc,  // kPoolReloc

    kAbs32,  // kPoolAbs32

    kRel32_A24,  // kPoolRel32
    kRel32_T8,
    kRel32_T11,
    kRel32_T21,
    kRel32_T24,

    kTypeCount
  };
};

struct AArch64ReferenceType {
  enum : uint8_t {
    kReloc,  // kPoolReloc

    kAbs32,  // kPoolAbs32

    kRel32_Immd14,  // kPoolRel32
    kRel32_Immd19,
    kRel32_Immd26,

    kTypeCount
  };
};

struct ELF32Traits {
  static constexpr Bitness kBitness = kBit32;

  using Elf_Shdr = Elf32_Shdr;
  using Elf_Phdr = Elf32_Phdr;
  using Elf_Ehdr = Elf32_Ehdr;
  using Elf_Rel = Elf32_Rel;
  using Elf_Rela = Elf32_Rela;

  // Architecture-specific definitions.
  static constexpr ExecutableType kIntelExeType = kExeTypeElfX86;
  static const char kIntelExeTypeString[];
  static constexpr e_machine_values kIntelMachineValue = EM_386;
  static constexpr uint32_t kIntelRelType = R_386_RELATIVE;
  using Rel32FinderIntelUse = Rel32FinderX86;

  static constexpr ExecutableType kARMExeType = kExeTypeElfArm32;
  static const char kARMExeTypeString[];
  static constexpr e_machine_values kARMMachineValue = EM_ARM;
  using ARMReferenceType = ARM32ReferenceType;
};

struct ELF64Traits {
  static constexpr Bitness kBitness = kBit64;

  using Elf_Shdr = Elf64_Shdr;
  using Elf_Phdr = Elf64_Phdr;
  using Elf_Ehdr = Elf64_Ehdr;
  using Elf_Rel = Elf64_Rel;
  using Elf_Rela = Elf64_Rela;

  // Architecture-specific definitions.
  static constexpr ExecutableType kIntelExeType = kExeTypeElfX64;
  static const char kIntelExeTypeString[];
  static constexpr e_machine_values kIntelMachineValue = EM_X86_64;
  static constexpr uint32_t kIntelRelType = R_X86_64_RELATIVE;
  using Rel32FinderIntelUse = Rel32FinderX64;

  static constexpr ExecutableType kARMExeType = kExeTypeElfAArch64;
  static const char kARMExeTypeString[];
  static constexpr e_machine_values kARMMachineValue = EM_AARCH64;
  using ARMReferenceType = AArch64ReferenceType;
};

// Disassembler for ELF.
template <class Traits>
class DisassemblerElf : public Disassembler {
 public:
  using HeaderVector = std::vector<const typename Traits::Elf_Shdr*>;

  DisassemblerElf();
  ~DisassemblerElf() override;

  // Disassembler:
  ExecutableType GetExeType() const override = 0;
  std::string GetExeTypeString() const override = 0;
  std::vector<ReferenceGroup> MakeReferenceGroups() const override = 0;

  // Find/Receive functions that are common among different architectures.
  std::unique_ptr<ReferenceReader> MakeFindRelocs(offset_t lo, offset_t hi);
  std::unique_ptr<ReferenceWriter> MakeReceiveRelocs(MutableBufferView image);
  std::unique_ptr<ReferenceReader> MakeFindAbs32(offset_t lo, offset_t hi);
  std::unique_ptr<ReferenceWriter> MakeReceiveAbs32(MutableBufferView image);

  const AddressTranslator& translator() const { return translator_; }

 protected:
  friend Disassembler;

  bool Parse(ConstBufferView image) override;

  // Returns the supported e_machine enum.
  virtual e_machine_values ElfEM() const = 0;

  // Returns the type to look for in the reloc section.
  virtual uint32_t GetRelType() const = 0;

  // Performs architecture-specific parsing of an executable section, to extract
  // rel32 references.
  virtual bool ParseExecSection(const typename Traits::Elf_Shdr& section) = 0;

  // Process rel32 data after extracting them from executable sections.
  virtual void PostProcessRel32() = 0;

  // The parsing routines below return true on success, and false on failure.
  // Parses ELF header and section headers, and performs basic validation.
  bool ParseHeader();

  // Extracts and stores section headers that we need.
  bool ExtractInterestingSectionHeaders();

  // Parsing functions that extract references from various sections.
  bool GetAbs32FromRelocSections();
  bool GetRel32FromCodeSections();
  bool ParseSections();

  // Main ELF header.
  const typename Traits::Elf_Ehdr* header_ = nullptr;

  // Section header table, ordered by section id.
  Elf32_Half sections_count_;
  const typename Traits::Elf_Shdr* sections_ = nullptr;

  // Program header table.
  Elf32_Half segments_count_;
  const typename Traits::Elf_Phdr* segments_ = nullptr;

  // Translator between offsets and RVAs.
  AddressTranslator translator_;

  // Extracted relocation section dimensions data, sorted by file offsets.
  std::vector<SectionDimsElf> reloc_section_dims_;

  // Headers of executable sections, sorted by file offsets.
  HeaderVector exec_headers_;

  // Sorted file offsets of abs32 locations.
  std::vector<offset_t> abs32_locations_;

 private:
  DISALLOW_COPY_AND_ASSIGN(DisassemblerElf);
};

// Disassembler for ELF with Intel architectures.
template <class Traits>
class DisassemblerElfIntel : public DisassemblerElf<Traits> {
 public:
  using Super = DisassemblerElf<Traits>;

  enum ReferenceType : uint8_t { kReloc, kAbs32, kRel32, kTypeCount };

  DisassemblerElfIntel();
  ~DisassemblerElfIntel() override;

  // Disassembler:
  ExecutableType GetExeType() const override;
  std::string GetExeTypeString() const override;
  std::vector<ReferenceGroup> MakeReferenceGroups() const override;

  // DisassemblerElf:
  e_machine_values ElfEM() const override;
  uint32_t GetRelType() const override;
  bool ParseExecSection(const typename Traits::Elf_Shdr& section) override;
  void PostProcessRel32() override;

  // Specialized Find/Receive functions.
  std::unique_ptr<ReferenceReader> MakeFindRel32(offset_t lower,
                                                 offset_t upper);
  std::unique_ptr<ReferenceWriter> MakeReceiveRel32(MutableBufferView image);

 private:
  // Sorted file offsets of rel32 locations.
  std::vector<offset_t> rel32_locations_;

  DISALLOW_COPY_AND_ASSIGN(DisassemblerElfIntel);
};

using DisassemblerElfX86 = DisassemblerElfIntel<ELF32Traits>;
using DisassemblerElfX64 = DisassemblerElfIntel<ELF64Traits>;

// Disassembler for ELF with ARM architectures.
template <class Traits>
class DisassemblerElfARM : public DisassemblerElf<Traits> {
 public:
  using Super = DisassemblerElf<Traits>;

  DisassemblerElfARM();
  ~DisassemblerElfARM() override;

  // Disassembler:
  ExecutableType GetExeType() const override;
  std::string GetExeTypeString() const override;
  std::vector<ReferenceGroup> MakeReferenceGroups() const override = 0;

  // Determines whether |offset| is in an exectuable section.
  bool IsOffsetInExecSection(offset_t offset);

  // DisassemblerElf:
  e_machine_values ElfEM() const override;
  uint32_t GetRelType() const override = 0;
  bool ParseExecSection(const typename Traits::Elf_Shdr& section) override = 0;
  void PostProcessRel32() override;

 protected:
  // Sorted file offsets of rel32 locations for each rel32 address type.
  std::vector<offset_t>
      rel32_locations_table_[Traits::ARMReferenceType::kTypeCount];

 private:
  DISALLOW_COPY_AND_ASSIGN(DisassemblerElfARM);
};

class DisassemblerElfARM32 : public DisassemblerElfARM<ELF32Traits> {
 public:
  using Super = DisassemblerElfARM<ELF32Traits>;

  DisassemblerElfARM32();
  ~DisassemblerElfARM32() override;

  // Disassembler:
  std::vector<ReferenceGroup> MakeReferenceGroups() const override;

  // DisassemblerElf:
  uint32_t GetRelType() const override;
  bool ParseExecSection(const typename ELF32Traits::Elf_Shdr& section) override;

  // Under the naive assumption that an exectuable section is entirely ARM mode
  // or THUMB2 mode, this function implements heuristics to distinguish between
  // the two. Returns true if section is THUMB2 mode; otherwise return false.
  bool IsExecSectionThumb2(const typename ELF32Traits::Elf_Shdr& section) const;

  // Specialized Find/Receive functions for different rel32 address types.
  std::unique_ptr<ReferenceReader> MakeFindRel32A24(offset_t lower,
                                                    offset_t upper);
  std::unique_ptr<ReferenceWriter> MakeReceiveRel32A24(MutableBufferView image);

  std::unique_ptr<ReferenceReader> MakeFindRel32T8(offset_t lower,
                                                   offset_t upper);
  std::unique_ptr<ReferenceWriter> MakeReceiveRel32T8(MutableBufferView image);

  std::unique_ptr<ReferenceReader> MakeFindRel32T11(offset_t lower,
                                                    offset_t upper);
  std::unique_ptr<ReferenceWriter> MakeReceiveRel32T11(MutableBufferView image);

  std::unique_ptr<ReferenceReader> MakeFindRel32T21(offset_t lower,
                                                    offset_t upper);
  std::unique_ptr<ReferenceWriter> MakeReceiveRel32T21(MutableBufferView image);

  std::unique_ptr<ReferenceReader> MakeFindRel32T24(offset_t lower,
                                                    offset_t upper);
  std::unique_ptr<ReferenceWriter> MakeReceiveRel32T24(MutableBufferView image);

 private:
  DISALLOW_COPY_AND_ASSIGN(DisassemblerElfARM32);
};

class DisassemblerElfAArch64 : public DisassemblerElfARM<ELF64Traits> {
 public:
  using Super = DisassemblerElfARM<ELF64Traits>;

  DisassemblerElfAArch64();
  ~DisassemblerElfAArch64() override;

  // Disassembler:
  std::vector<ReferenceGroup> MakeReferenceGroups() const override;

  // DisassemblerElf:
  uint32_t GetRelType() const override;
  bool ParseExecSection(const typename ELF64Traits::Elf_Shdr& section) override;

  // Specialized Find/Receive functions for different rel32 address types.
  std::unique_ptr<ReferenceReader> MakeFindRel32Immd14(offset_t lower,
                                                       offset_t upper);
  std::unique_ptr<ReferenceWriter> MakeReceiveRel32Immd14(
      MutableBufferView image);

  std::unique_ptr<ReferenceReader> MakeFindRel32Immd19(offset_t lower,
                                                       offset_t upper);
  std::unique_ptr<ReferenceWriter> MakeReceiveRel32Immd19(
      MutableBufferView image);

  std::unique_ptr<ReferenceReader> MakeFindRel32Immd26(offset_t lower,
                                                       offset_t upper);
  std::unique_ptr<ReferenceWriter> MakeReceiveRel32Immd26(
      MutableBufferView image);

 private:
  DISALLOW_COPY_AND_ASSIGN(DisassemblerElfAArch64);
};

// Heuristic type detection for ELF. Returns |kExeTypeUnknown| if |image| is not
// an ELF file, or is an ELF file for an unrecognized architecture type.
// Otherwise returns an ExecutableType as a potential ELF type for |image|,
// although further tests are needed.
ExecutableType QuickDetectElf(ConstBufferView image);

}  // namespace zucchini

#endif  // CHROME_INSTALLER_ZUCCHINI_DISASSEMBLER_ELF_H_
