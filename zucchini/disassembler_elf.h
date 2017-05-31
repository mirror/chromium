// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZUCCHINI_DISASSEMBLER_ELF_H_
#define ZUCCHINI_DISASSEMBLER_ELF_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "base/macros.h"
#include "zucchini/disassembler.h"
#include "zucchini/image_utils.h"
#include "zucchini/ranges/functional.h"
#include "zucchini/region.h"
#include "zucchini/rel32_finder.h"
#include "zucchini/rel32_utils.h"
#include "zucchini/type_elf.h"

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
  typedef struct Elf32_Shdr Elf_Shdr;
  typedef struct Elf32_Phdr Elf_Phdr;
  typedef struct Elf32_Ehdr Elf_Ehdr;
  typedef struct Elf32_Rel Elf_Rel;
  typedef struct Elf32_Rela Elf_Rela;
  // Architecture-specific definitions.
  typedef struct ARM32ReferenceType ARMReferenceType;
  static constexpr e_machine_values kARMMachineValue = EM_ARM;
  static constexpr ExecutableType kARMExeType = EXE_TYPE_ELF_ARM32;
  static const char kARMExeTypeString[];
};

struct ELF64Traits {
  typedef struct Elf64_Shdr Elf_Shdr;
  typedef struct Elf64_Phdr Elf_Phdr;
  typedef struct Elf64_Ehdr Elf_Ehdr;
  typedef struct Elf64_Rel Elf_Rel;
  typedef struct Elf64_Rela Elf_Rela;
  // Architecture-specific definitions.
  typedef struct AArch64ReferenceType ARMReferenceType;
  static constexpr e_machine_values kARMMachineValue = EM_AARCH64;
  static constexpr ExecutableType kARMExeType = EXE_TYPE_ELF_AARCH64;
  static const char kARMExeTypeString[];
};

// Disassembler for ELF.
template <typename TRAITS>
class DisassemblerElf : public Disassembler {
 public:
  using Traits = TRAITS;

  explicit DisassemblerElf(Region image);
  ~DisassemblerElf() override;

  // Quick executable detection for General ELF, to be used by architecture-
  // specific checks, which supply this with more parameters.
  static bool QuickDetect(Region image, e_machine_values e_machine);

  // Disassembler:
  ExecutableType GetExeType() const override = 0;
  std::string GetExeTypeString() const override = 0;
  ReferenceTraits GetReferenceTraits(uint8_t type) const override = 0;
  uint8_t GetReferenceTraitsCount() const override = 0;
  bool Parse() override;

  // AddressTranslator:
  RVAToOffsetTranslator GetRVAToOffsetTranslator() const override;
  OffsetToRVATranslator GetOffsetToRVATranslator() const override;

  // Helper for FindRelocs(): If |rel| contains |r_offset| for a supported type,
  // returns the RVA. Otherwise returns kNullRva.
  rva_t RelocTarget(typename TRAITS::Elf_Rel rel) const;

  // Find/Receive functions that are common among different architectures.
  ReferenceGenerator FindRelocs(offset_t lower, offset_t upper);
  ReferenceReceptor ReceiveRelocs();
  ReferenceGenerator FindAbs32(offset_t lower, offset_t upper);
  ReferenceReceptor ReceiveAbs32();

 protected:
  // Returns the supported e_machine enum.
  virtual e_machine_values ElfEM() const = 0;

  // Returns whether a reloc with |rel_type| should be extracted.
  virtual bool IsElfRelTypeSupported(uint32_t rel_type) const = 0;

  // Performs architecture-specific parsing of an executable section, to extract
  // rel32 references.
  virtual bool ParseExecSection(const typename TRAITS::Elf_Shdr& section) = 0;

  // Process rel32 data after extracting them from executable sections.
  virtual void PostProcessRel32() = 0;

  // Address translation functions.
  const typename TRAITS::Elf_Shdr* RVAToSection(rva_t rva) const;
  const typename TRAITS::Elf_Shdr* OffsetToSection(offset_t offset) const;
  offset_t RVAToOffset(rva_t rva) const;
  rva_t OffsetToRVA(offset_t offset) const;

  // The parsing routines below return true on success, and false on failure.
  // Parses ELF header and section headers, and performs basic validation.
  bool ParseHeader();

  // Determines whether |section| is a reloc section.
  bool IsRelocSection(const typename TRAITS::Elf_Shdr& section);

  // Determines whether |section| is a section with executable code.
  bool IsExecSection(const typename TRAITS::Elf_Shdr& section);

  // Extracts and stores section headers that we need.
  bool ExtractInterestingSectionHeaders();

  // Parsing functions that extract references from various sections.
  bool GetAbs32FromRelocSections();
  bool GetRel32FromCodeSections();
  bool ParseSections();

  // Main ELF header.
  const typename TRAITS::Elf_Ehdr* header_ = nullptr;

  // Section header table, ordered by section id.
  Elf32_Half sections_count_;
  const typename TRAITS::Elf_Shdr* sections_ = nullptr;

  // Program header table.
  Elf32_Half segments_count_;
  const typename TRAITS::Elf_Phdr* segments_ = nullptr;

  // Sorted tables to map from RVA / file offset to sections. We exclude empty
  // sections and sections with RVA of 0.
  std::vector<std::pair<rva_t, int>> section_rva_map_;
  std::vector<std::pair<offset_t, int>> section_offset_map_;

  typedef typename std::vector<const typename TRAITS::Elf_Shdr*> HeaderVector;

  // Headers of relocation sections, sorted by file offsets.
  HeaderVector reloc_headers_;

  // Headers of executable sections, sorted by file offsets.
  HeaderVector exec_headers_;

  // Sorted file offsets of abs32 locations.
  std::vector<offset_t> abs32_locations_;

 private:
  DISALLOW_COPY_AND_ASSIGN(DisassemblerElf);
};

// Disassembler for ELF, specialized for x86.
class DisassemblerElfX86 : public DisassemblerElf<ELF32Traits> {
 public:
  using Super = DisassemblerElf<ELF32Traits>;

  enum ReferenceType : uint8_t { kReloc, kAbs32, kRel32, kTypeCount };
  static const ReferenceTraits kGroups[];

  explicit DisassemblerElfX86(Region image);
  ~DisassemblerElfX86() override;

  // Applies quick checks to determine if |image| *may* point to the start of an
  // executable. Returns true on success.
  static bool QuickDetect(Region image);

  // Disassembler:
  ExecutableType GetExeType() const override;
  std::string GetExeTypeString() const override;
  ReferenceTraits GetReferenceTraits(uint8_t type) const override;
  uint8_t GetReferenceTraitsCount() const override;

  // DisassemblerElf:
  e_machine_values ElfEM() const override;
  bool IsElfRelTypeSupported(uint32_t rel_type) const override;
  bool ParseExecSection(const typename ELF32Traits::Elf_Shdr& section) override;
  void PostProcessRel32() override;

  // Specialized Find/Receive functions.
  ReferenceGenerator FindRel32(offset_t lower, offset_t upper);
  ReferenceReceptor ReceiveRel32();

 private:
  // Sorted file offsets of rel32 locations.
  std::vector<offset_t> rel32_locations_;

  DISALLOW_COPY_AND_ASSIGN(DisassemblerElfX86);
};

// Disassembler for ELF.
template <typename TRAITS>
class DisassemblerElfARM : public DisassemblerElf<TRAITS> {
 public:
  using Super = DisassemblerElf<TRAITS>;

  explicit DisassemblerElfARM(Region image);
  ~DisassemblerElfARM() override;

  // Applies quick checks to determine if |image| *may* point to the start of an
  // executable. Returns true on success.
  static bool QuickDetect(Region image);

  // Disassembler:
  ExecutableType GetExeType() const override;
  std::string GetExeTypeString() const override;
  ReferenceTraits GetReferenceTraits(uint8_t type) const override = 0;
  uint8_t GetReferenceTraitsCount() const override = 0;

  // Determines whether |offset| is in an exectuable section.
  bool IsOffsetInExecSection(offset_t offset);

  // DisassemblerElf:
  e_machine_values ElfEM() const override;
  bool IsElfRelTypeSupported(uint32_t rel_type) const override = 0;
  bool ParseExecSection(const typename TRAITS::Elf_Shdr& section) override = 0;
  void PostProcessRel32() override;

 protected:
  // Sorted file offsets of rel32 locations for each rel32 address type.
  std::vector<offset_t>
      rel32_locations_table_[TRAITS::ARMReferenceType::kTypeCount];

 private:
  DISALLOW_COPY_AND_ASSIGN(DisassemblerElfARM);
};

class DisassemblerElfARM32 : public DisassemblerElfARM<ELF32Traits> {
 public:
  using Super = DisassemblerElfARM<ELF32Traits>;

  explicit DisassemblerElfARM32(Region image) : Super(image) {}
  ~DisassemblerElfARM32() override;

  // Disassembler:
  ReferenceTraits GetReferenceTraits(uint8_t type) const override;
  uint8_t GetReferenceTraitsCount() const override;

  // DisassemblerElf:
  bool IsElfRelTypeSupported(uint32_t rel_type) const override;
  bool ParseExecSection(const typename ELF32Traits::Elf_Shdr& section) override;

  // Under the naive assumption that an exectuable section is entirely ARM mode
  // or THUMB2 mode, this function implements heuristics to distinguish between
  // the two. Returns true if section is THUMB2 mode; otherwise return false.
  bool IsExecSectionThumb2(const typename ELF32Traits::Elf_Shdr& section) const;

  // Specialized Find/Receive functions for different rel32 address types.
  ReferenceGenerator FindRel32A24(offset_t lower, offset_t upper);
  ReferenceReceptor ReceiveRel32A24();

  ReferenceGenerator FindRel32T8(offset_t lower, offset_t upper);
  ReferenceReceptor ReceiveRel32T8();

  ReferenceGenerator FindRel32T11(offset_t lower, offset_t upper);
  ReferenceReceptor ReceiveRel32T11();

  ReferenceGenerator FindRel32T21(offset_t lower, offset_t upper);
  ReferenceReceptor ReceiveRel32T21();

  ReferenceGenerator FindRel32T24(offset_t lower, offset_t upper);
  ReferenceReceptor ReceiveRel32T24();

 private:
  DISALLOW_COPY_AND_ASSIGN(DisassemblerElfARM32);
};

class DisassemblerElfAArch64 : public DisassemblerElfARM<ELF64Traits> {
 public:
  using Super = DisassemblerElfARM<ELF64Traits>;

  explicit DisassemblerElfAArch64(Region image) : Super(image) {}
  ~DisassemblerElfAArch64() override;

  // Disassembler:
  ReferenceTraits GetReferenceTraits(uint8_t type) const override;
  uint8_t GetReferenceTraitsCount() const override;

  // DisassemblerElf:
  bool IsElfRelTypeSupported(uint32_t rel_type) const override;
  bool ParseExecSection(const typename ELF64Traits::Elf_Shdr& section) override;

  // Specialized Find/Receive functions for different rel32 address types.
  ReferenceGenerator FindRel32Immd14(offset_t lower, offset_t upper);
  ReferenceReceptor ReceiveRel32Immd14();

  ReferenceGenerator FindRel32Immd19(offset_t lower, offset_t upper);
  ReferenceReceptor ReceiveRel32Immd19();

  ReferenceGenerator FindRel32Immd26(offset_t lower, offset_t upper);
  ReferenceReceptor ReceiveRel32Immd26();

 private:
  DISALLOW_COPY_AND_ASSIGN(DisassemblerElfAArch64);
};

}  // namespace zucchini

#endif  // ZUCCHINI_DISASSEMBLER_ELF_H_
