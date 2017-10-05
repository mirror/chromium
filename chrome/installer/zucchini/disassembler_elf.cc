// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/disassembler_elf.h"

#include <stddef.h>

#include <algorithm>
#include <iostream>
#include <utility>

#include "base/logging.h"
#include "base/strings/stringprintf.h"
#include "chrome/installer/zucchini/algorithm.h"
#include "chrome/installer/zucchini/arm_utils.h"
#include "chrome/installer/zucchini/buffer_source.h"
#include "chrome/installer/zucchini/io_utils.h"

namespace zucchini {

namespace {

double kARM32BitCondAlwaysDensityThreshold = 0.4;

// Determines whether |section| is a reloc section.
template <class Traits>
bool IsRelocSection(const typename Traits::Elf_Shdr& section) {
  if (section.sh_size == 0)
    return false;
  if (section.sh_type == SHT_REL) {
    // Also validate |section.sh_entsize|, which gets used later.
    return section.sh_entsize == sizeof(typename Traits::Elf_Rel);
  }
  if (section.sh_type == SHT_RELA) {
    return section.sh_entsize == sizeof(typename Traits::Elf_Rela);
  }
  return false;
}

// Determines whether |section| is a section with executable code.
template <class Traits>
bool IsExecSection(const typename Traits::Elf_Shdr& section) {
  return (section.sh_flags & SHF_EXECINSTR) != 0;
}

}  // namespace

/******** ELF32Traits ********/

// static
constexpr e_machine_values ELF32Traits::kARMMachineValue;
constexpr ExecutableType ELF32Traits::kARMExeType;
const char ELF32Traits::kARMExeTypeString[] = "ELF ARM";

/******** ELF64Traits ********/

// static
constexpr e_machine_values ELF64Traits::kARMMachineValue;
constexpr ExecutableType ELF64Traits::kARMExeType;
const char ELF64Traits::kARMExeTypeString[] = "ELF ARM64";

/******** DisassemblerElf ********/

template <class Traits>
DisassemblerElf<Traits>::DisassemblerElf() = default;

template <class Traits>
DisassemblerElf<Traits>::~DisassemblerElf() = default;

template <class Traits>
bool DisassemblerElf<Traits>::Parse(ConstBufferView image) {
  image_ = image;
  return ParseHeader() && ParseSections();
}

template <class Traits>
std::unique_ptr<ReferenceReader> DisassemblerElf<Traits>::MakeFindRelocs(
    offset_t lo,
    offset_t hi) {
  DCHECK_LE(lo, hi);
  DCHECK_LE(hi, image_.size());

  if (reloc_section_dims_.empty())
    return base::MakeUnique<EmptyReferenceReader>();

  return base::MakeUnique<RelocReaderElf>(image_, Traits::kBits,
                                          reloc_section_dims_, GetRelType(), lo,
                                          hi, translator_);
}

template <class Traits>
std::unique_ptr<ReferenceWriter> DisassemblerElf<Traits>::MakeReceiveRelocs(
    MutableBufferView image) {
  return base::MakeUnique<RelocWriterElf>(image, Traits::kBits, translator_);
}

template <class Traits>
std::unique_ptr<ReferenceReader> DisassemblerElf<Traits>::MakeFindAbs32(
    offset_t lower,
    offset_t upper) {
  class ReferenceReaderImpl : public ReferenceReader {
   public:
    ReferenceReaderImpl(const DisassemblerElf* self,
                        ConstBufferView image,
                        offset_t lower,
                        offset_t upper)
        : image_(image), upper_(upper) {
      cur_it_ = std::lower_bound(self->abs32_locations_.begin(),
                                 self->abs32_locations_.end(), lower);
      abs32_locations_end_ = self->abs32_locations_.end();
    }
    base::Optional<Reference> GetNext() override {
      while (cur_it_ < abs32_locations_end_ && *cur_it_ < upper_) {
        offset_t location = *(cur_it_++);
        // |target| will not be dereferenced, so we don't worry about it
        // exceeding |image_.size()| (in fact, there are valid cases where it
        // does).
        offset_t target = image_.read<uint32_t>(location);
        if (target == kInvalidOffset) {
          continue;
        }
        // However, in rare cases, the most significant bit of |target| is set.
        // This interferes with label marking. A quick fix is to reject these.
        if (IsMarked(target)) {
          LOG(WARNING) << "Warning: Skipping mark-aliased ELF abs32 target: "
                       << AsHex<8>(location) << " -> " << AsHex<8>(target)
                       << ".";
          continue;
        }
        return Reference{location, target};
      }
      return base::nullopt;
    }

   private:
    ConstBufferView image_;
    std::vector<offset_t>::const_iterator cur_it_;
    std::vector<offset_t>::const_iterator abs32_locations_end_;
    offset_t upper_;
  };

  return base::MakeUnique<ReferenceReaderImpl>(this, image_, lower, upper);
}

template <class Traits>
std::unique_ptr<ReferenceWriter> DisassemblerElf<Traits>::MakeReceiveAbs32(
    MutableBufferView image) {
  class ReferenceWriterImpl : public ReferenceWriter {
   public:
    explicit ReferenceWriterImpl(MutableBufferView image) : image_(image) {}
    void PutNext(Reference ref) override {
      image_.write<uint32_t>(ref.location, ref.target);
    }

   private:
    MutableBufferView image_;
  };

  return base::MakeUnique<ReferenceWriterImpl>(image);
}

template <class Traits>
bool DisassemblerElf<Traits>::ParseHeader() {
  BufferSource source(image_);

  if (!source.CheckNextBytes({0x7F, 'E', 'L', 'F'}))
    return false;

  header_ = source.GetPointer<typename Traits::Elf_Ehdr>();
  if (!header_)  // Rewind: Magic is part of header!
    return false;

  if (header_->e_type != ET_EXEC && header_->e_type != ET_DYN)
    return false;

  if (header_->e_machine != ElfEM())
    return false;

  if (header_->e_version != 1)
    return false;

  if (header_->e_shentsize != sizeof(typename Traits::Elf_Shdr))
    return false;

  sections_count_ = header_->e_shnum;
  source = std::move(BufferSource(image_).Skip(header_->e_shoff));
  sections_ = source.GetArray<typename Traits::Elf_Shdr>(sections_count_);
  if (!sections_)
    return false;

  segments_count_ = header_->e_phnum;
  source = std::move(BufferSource(image_).Skip(header_->e_phoff));
  segments_ = source.GetArray<typename Traits::Elf_Phdr>(segments_count_);
  if (!segments_)
    return false;

  // Check string section -- even though we've stopped using them.
  Elf32_Half string_section_id = header_->e_shstrndx;
  if (string_section_id >= sections_count_)
    return false;
  size_t section_names_size = sections_[string_section_id].sh_size;
  if (section_names_size > 0) {
    // If nonempty, then last byte of string section must be null.
    const char* section_names = nullptr;
    source = std::move(
        BufferSource(image_).Skip(sections_[string_section_id].sh_offset));
    section_names = source.GetArray<char>(section_names_size);
    if (!section_names) {
      return false;
    }
    if (section_names[section_names_size - 1] != '\0')
      return false;
  }

  // Establish bound on encountered offsets.
  offset_t offset_bound = 0;
  auto section_table_end =
      header_->e_shoff + (sections_count_ * sizeof(typename Traits::Elf_Shdr));
  CHECK_LE(section_table_end, image_.size());
  auto segment_table_end =
      header_->e_phoff + (segments_count_ * sizeof(typename Traits::Elf_Phdr));
  CHECK_LE(segment_table_end, image_.size());
  offset_bound =
      std::max(offset_bound, static_cast<offset_t>(section_table_end));
  offset_bound =
      std::max(offset_bound, static_cast<offset_t>(segment_table_end));

  // Visit each section, validate, and add address translation data to |units|.
  std::vector<AddressTranslator::Unit> units;
  units.reserve(sections_count_);

  for (int i = 0; i < sections_count_; ++i) {
    const typename Traits::Elf_Shdr* section = &sections_[i];

    // Skip empty sections. These don't affect |offset_bound|, and don't
    // contribute to RVA-offset mapping.
    if (section->sh_size == 0)
      continue;

    // Be lax with RVAs: Assume they fit in int32_t, even for 64-bit. If
    // assumption fails, simply skip the section with warning.
    if (!RangeIsBounded(section->sh_addr, section->sh_size, kRvaBound)) {
      LOG(WARNING) << "Section " << i << " does not fit in int32_t.";
      continue;
    }

    // Update |offset_bound|.
    if (section->sh_type != SHT_NOBITS) {
      // Be strict with offsets: Any size overflow invalidates the file.
      if (!image_.covers({section->sh_offset, section->sh_size}))
        return false;

      offset_t section_end = section->sh_offset + section->sh_size;
      CHECK_LE(section_end, image_.size());
      offset_bound = std::max(offset_bound, section_end);
    }

    // Compute mappings to translate between RVA and offset. As a heuristic,
    // sections with RVA == 0 (i.e., |sh_addr == 0|) are ignored because these
    // tend to be duplicates (which cause problems during lookup), and tend to
    // be uninteresting.
    if (section->sh_addr > 0) {
      // Add |section| data for offset-RVA translation.
      units.push_back({section->sh_offset, section->sh_size, section->sh_addr,
                       section->sh_size});
    }
  }

  // Initialize |translator_| for offset-RVA translations. Any inconsistency
  // (e.g., 2 offsets correspond to the same RVA) would invalidate the PE file.
  if (translator_.Initialize(std::move(units)) != AddressTranslator::kSuccess)
    return false;

  // Visits |segments_| to get better estimate on |offset_bound|.
  for (const typename Traits::Elf_Phdr* segment = segments_;
       segment != segments_ + segments_count_; ++segment) {
    if (!RangeIsBounded(segment->p_offset, segment->p_filesz, kOffsetBound))
      return false;
    offset_t segment_end = segment->p_offset + segment->p_filesz;
    offset_bound = std::max(offset_bound, segment_end);
  }

  if (offset_bound > image_.size())
    return false;
  image_.shrink(offset_bound);

  return true;
}

template <class Traits>
bool DisassemblerElf<Traits>::ExtractInterestingSectionHeaders() {
  DCHECK(reloc_section_dims_.empty());
  DCHECK(exec_headers_.empty());
  for (Elf32_Half i = 0; i < sections_count_; ++i) {
    const typename Traits::Elf_Shdr* section = sections_ + i;
    if (IsRelocSection<Traits>(*section))
      reloc_section_dims_.emplace_back(section);
    else if (IsExecSection<Traits>(*section))
      exec_headers_.push_back(section);
  }
  auto comp = [](const typename Traits::Elf_Shdr* a,
                 const typename Traits::Elf_Shdr* b) {
    return a->sh_offset < b->sh_offset;
  };
  std::sort(reloc_section_dims_.begin(), reloc_section_dims_.end());
  std::sort(exec_headers_.begin(), exec_headers_.end(), comp);
  return true;
}

template <class Traits>
bool DisassemblerElf<Traits>::GetAbs32FromRelocSections() {
  constexpr int kAbs32Width = 4;
  DCHECK(abs32_locations_.empty());
  auto relocs = MakeFindRelocs(0, offset_t(size()));
  for (auto ref = relocs->GetNext(); ref; ref = relocs->GetNext()) {
    // Reject null targets and targets outside |image_|.
    if (ref->target > 0 &&
        CheckDataFit(ref->target, kAbs32Width, image_.size()))
      abs32_locations_.push_back(ref->target);
  }
  abs32_locations_.shrink_to_fit();
  std::sort(abs32_locations_.begin(), abs32_locations_.end());
  return true;
}

template <class Traits>
bool DisassemblerElf<Traits>::GetRel32FromCodeSections() {
  for (const typename Traits::Elf_Shdr* section : exec_headers_)
    ParseExecSection(*section);
  PostProcessRel32();
  return true;
}

template <class Traits>
bool DisassemblerElf<Traits>::ParseSections() {
  if (!ExtractInterestingSectionHeaders())
    return false;
  if (!GetAbs32FromRelocSections())
    return false;
  if (!GetRel32FromCodeSections())
    return false;
  return true;
}

/******** DisassemblerElfX86 ********/

DisassemblerElfX86::DisassemblerElfX86() = default;
DisassemblerElfX86::~DisassemblerElfX86() = default;

ExecutableType DisassemblerElfX86::GetExeType() const {
  return kExeTypeElfX86;
}

std::string DisassemblerElfX86::GetExeTypeString() const {
  return "ELF x86";
}

std::vector<ReferenceGroup> DisassemblerElfX86::MakeReferenceGroups() const {
  return {{ReferenceTypeTraits{4, TypeTag(kReloc), PoolTag(kReloc)},
           &DisassemblerElfX86::MakeFindRelocs,
           &DisassemblerElfX86::MakeReceiveRelocs},
          {ReferenceTypeTraits{4, TypeTag(kAbs32), PoolTag(kAbs32)},
           &DisassemblerElfX86::MakeFindAbs32,
           &DisassemblerElfX86::MakeReceiveAbs32},
          {ReferenceTypeTraits{4, TypeTag(kRel32), PoolTag(kRel32)},
           &DisassemblerElfX86::MakeFindRel32,
           &DisassemblerElfX86::MakeReceiveRel32}};
}

e_machine_values DisassemblerElfX86::ElfEM() const {
  return EM_386;
}

uint32_t DisassemblerElfX86::GetRelType() const {
  return R_386_RELATIVE;
}

bool DisassemblerElfX86::ParseExecSection(
    const typename ELF32Traits::Elf_Shdr& section) {
  std::ptrdiff_t from_offset_to_rva = section.sh_addr - section.sh_offset;
  rva_t start_rva = section.sh_addr;
  rva_t end_rva = start_rva + section.sh_size;

  ConstBufferView region(image_.begin() + section.sh_offset, section.sh_size);
  Abs32GapFinder gap_finder(image_, region, abs32_locations_, 4);
  Rel32FinderX86 finder(image_);
  for (auto gap = gap_finder.GetNext(); gap.has_value();
       gap = gap_finder.GetNext()) {
    finder.Reset(gap.value());
    for (auto rel32 = finder.GetNext(); rel32.has_value();
         rel32 = finder.GetNext()) {
      offset_t rel32_offset = offset_t(rel32->location - image_.begin());
      rva_t rel32_rva = rva_t(rel32_offset + from_offset_to_rva);
      rva_t target_rva = rel32_rva + 4 + image_.read<uint32_t>(rel32_offset);
      if (target_rva < image_.size() &&  // Subsumes rva != kUnassignedRva.
          (rel32->can_point_outside_section ||
           (start_rva <= target_rva && target_rva < end_rva))) {
        finder.Accept();
        rel32_locations_.push_back(rel32_offset);
      }
    }
  }
  return true;
}

void DisassemblerElfX86::PostProcessRel32() {
  rel32_locations_.shrink_to_fit();
  std::sort(rel32_locations_.begin(), rel32_locations_.end());
}

std::unique_ptr<ReferenceReader> DisassemblerElfX86::MakeFindRel32(
    offset_t lower,
    offset_t upper) {
  return base::MakeUnique<Rel32ReaderX86>(image_, lower, upper,
                                          &rel32_locations_, translator_);
}

std::unique_ptr<ReferenceWriter> DisassemblerElfX86::MakeReceiveRel32(
    MutableBufferView image) {
  return base::MakeUnique<Rel32WriterX86>(image, translator_);
}

/******** DisassemblerElfARM ********/

template <class Traits>
DisassemblerElfARM<Traits>::DisassemblerElfARM() = default;

template <class Traits>
DisassemblerElfARM<Traits>::~DisassemblerElfARM() = default;

template <class Traits>
ExecutableType DisassemblerElfARM<Traits>::GetExeType() const {
  return Traits::kARMExeType;
}

template <class Traits>
std::string DisassemblerElfARM<Traits>::GetExeTypeString() const {
  return Traits::kARMExeTypeString;
}

template <class Traits>
bool DisassemblerElfARM<Traits>::IsOffsetInExecSection(offset_t offset) {
  for (const typename Traits::Elf_Shdr* section : Super::exec_headers_) {
    if (offset >= section->sh_offset &&
        offset < section->sh_offset + section->sh_size) {
      return true;
    }
  }
  return false;
}

template <class Traits>
e_machine_values DisassemblerElfARM<Traits>::ElfEM() const {
  return Traits::kARMMachineValue;
}

template <class Traits>
void DisassemblerElfARM<Traits>::PostProcessRel32() {
  for (int type = 0; type < ARM32Rel32Parser::NUM_ADDR_TYPE; ++type) {
    std::sort(rel32_locations_table_[type].begin(),
              rel32_locations_table_[type].end());
  }
}

template class DisassemblerElfARM<ELF32Traits>;
template class DisassemblerElfARM<ELF64Traits>;

/******** DisassemblerElfARM32 ********/

DisassemblerElfARM32::DisassemblerElfARM32() = default;
DisassemblerElfARM32::~DisassemblerElfARM32() = default;

std::vector<ReferenceGroup> DisassemblerElfARM32::MakeReferenceGroups() const {
  // Initialized on first use.
  return {
      {ReferenceTypeTraits{4, TypeTag(ARM32ReferenceType::kReloc),
                           PoolTag(ARMReferencePool::kPoolReloc)},
       &Super::MakeFindRelocs, &Super::MakeReceiveRelocs},
      {ReferenceTypeTraits{4, TypeTag(ARM32ReferenceType::kAbs32),
                           PoolTag(ARMReferencePool::kPoolAbs32)},
       &Super::MakeFindAbs32, &Super::MakeReceiveAbs32},
      {ReferenceTypeTraits{4, TypeTag(ARM32ReferenceType::kRel32_A24),
                           PoolTag(ARMReferencePool::kPoolRel32)},
       &DisassemblerElfARM32::MakeFindRel32A24,
       &DisassemblerElfARM32::MakeReceiveRel32A24},
      {ReferenceTypeTraits{2, TypeTag(ARM32ReferenceType::kRel32_T8),
                           PoolTag(ARMReferencePool::kPoolRel32)},
       &DisassemblerElfARM32::MakeFindRel32T8,
       &DisassemblerElfARM32::MakeReceiveRel32T8},
      {ReferenceTypeTraits{2, TypeTag(ARM32ReferenceType::kRel32_T11),
                           PoolTag(ARMReferencePool::kPoolRel32)},
       &DisassemblerElfARM32::MakeFindRel32T11,
       &DisassemblerElfARM32::MakeReceiveRel32T11},
      {ReferenceTypeTraits{4, TypeTag(ARM32ReferenceType::kRel32_T21),
                           PoolTag(ARMReferencePool::kPoolRel32)},
       &DisassemblerElfARM32::MakeFindRel32T21,
       &DisassemblerElfARM32::MakeReceiveRel32T21},
      {ReferenceTypeTraits{4, TypeTag(ARM32ReferenceType::kRel32_T24),
                           PoolTag(ARMReferencePool::kPoolRel32)},
       &DisassemblerElfARM32::MakeFindRel32T24,
       &DisassemblerElfARM32::MakeReceiveRel32T24},
  };
}

uint32_t DisassemblerElfARM32::GetRelType() const {
  return R_ARM_RELATIVE;
}

bool DisassemblerElfARM32::ParseExecSection(
    const typename ELF32Traits::Elf_Shdr& section) {
  ConstBufferView region(image_.begin() + section.sh_offset, section.sh_size);
  Abs32GapFinder gap_finder(image_, region, abs32_locations_, 4);
  Rel32FinderARM32 finder(image_, translator_);
  finder.SetIsThumb2(IsExecSectionThumb2(section));
  for (auto gap = gap_finder.GetNext(); gap.has_value();
       gap = gap_finder.GetNext()) {
    finder.Reset(gap.value());
    for (auto rel32 = finder.GetNext(); rel32.has_value();
         rel32 = finder.GetNext()) {
      // False detection is still possible. For example, we may misidentify ARM
      // vs. THUMB2 mode, or fetch non-code data. This is a quick test to
      // eliminate some of these.
      if (IsOffsetInExecSection(rel32->target_offset)) {
        finder.Accept();
        rel32_locations_table_[rel32->type].push_back(rel32->offset);
      }
    }
  }
  return true;
}

bool DisassemblerElfARM32::IsExecSectionThumb2(
    const typename ELF32Traits::Elf_Shdr& section) const {
  // ARM mode requires 4-byte alignment.
  if (section.sh_addr % 4 != 0)
    return true;
  if (section.sh_size % 4 != 0)
    return true;
  const uint8_t* first = image_.begin() + section.sh_offset;
  const uint8_t* end = first + section.sh_size;
  // Each instruction in 32-bit ARM (little-endian) looks like
  //   ?? ?? ?? X?,
  // where X specifies conditional execution. X = 0xE represents AL = "ALways
  // execute", and tends to appear very often. We use this as our main indicator
  // to discern 32-bit ARM mode from THUMB2 mode.
  size_t num = 0;
  size_t den = 0;
  for (const uint8_t* cur = first; cur < end; cur += 4) {
    uint8_t maybe_cond = cur[3] & 0xF0;
    if (maybe_cond == 0xE0)
      ++num;
    ++den;
  }

  std::cout << "Section scan: " << num << " / " << den << " => "
            << base::StringPrintf("%.2f", num * 100.0 / den) << "%"
            << std::endl;
  return num < den * kARM32BitCondAlwaysDensityThreshold;
}

std::unique_ptr<ReferenceReader> DisassemblerElfARM32::MakeFindRel32A24(
    offset_t lower,
    offset_t upper) {
  return ARMCreateFindRel32<ARM32Rel32Parser::AddrTraits_A24>(
      translator_, image_.begin(),
      rel32_locations_table_[ARM32Rel32Parser::ADDR_A24], lower, upper);
}

std::unique_ptr<ReferenceWriter> DisassemblerElfARM32::MakeReceiveRel32A24(
    MutableBufferView image) {
  return ARMCreateReceiveRel32<ARM32Rel32Parser::AddrTraits_A24>(translator_,
                                                                 image.begin());
}

std::unique_ptr<ReferenceReader> DisassemblerElfARM32::MakeFindRel32T8(
    offset_t lower,
    offset_t upper) {
  return ARMCreateFindRel32<ARM32Rel32Parser::AddrTraits_T8>(
      translator_, image_.begin(),
      rel32_locations_table_[ARM32Rel32Parser::ADDR_T8], lower, upper);
}

std::unique_ptr<ReferenceWriter> DisassemblerElfARM32::MakeReceiveRel32T8(
    MutableBufferView image) {
  return ARMCreateReceiveRel32<ARM32Rel32Parser::AddrTraits_T8>(translator_,
                                                                image.begin());
}

std::unique_ptr<ReferenceReader> DisassemblerElfARM32::MakeFindRel32T11(
    offset_t lower,
    offset_t upper) {
  return ARMCreateFindRel32<ARM32Rel32Parser::AddrTraits_T11>(
      translator_, image_.begin(),
      rel32_locations_table_[ARM32Rel32Parser::ADDR_T11], lower, upper);
}

std::unique_ptr<ReferenceWriter> DisassemblerElfARM32::MakeReceiveRel32T11(
    MutableBufferView image) {
  return ARMCreateReceiveRel32<ARM32Rel32Parser::AddrTraits_T11>(translator_,
                                                                 image.begin());
}

std::unique_ptr<ReferenceReader> DisassemblerElfARM32::MakeFindRel32T21(
    offset_t lower,
    offset_t upper) {
  return ARMCreateFindRel32<ARM32Rel32Parser::AddrTraits_T21>(
      translator_, image_.begin(),
      rel32_locations_table_[ARM32Rel32Parser::ADDR_T21], lower, upper);
}

std::unique_ptr<ReferenceWriter> DisassemblerElfARM32::MakeReceiveRel32T21(
    MutableBufferView image) {
  return ARMCreateReceiveRel32<ARM32Rel32Parser::AddrTraits_T21>(translator_,
                                                                 image.begin());
}

std::unique_ptr<ReferenceReader> DisassemblerElfARM32::MakeFindRel32T24(
    offset_t lower,
    offset_t upper) {
  return ARMCreateFindRel32<ARM32Rel32Parser::AddrTraits_T24>(
      translator_, image_.begin(),
      rel32_locations_table_[ARM32Rel32Parser::ADDR_T24], lower, upper);
}

std::unique_ptr<ReferenceWriter> DisassemblerElfARM32::MakeReceiveRel32T24(
    MutableBufferView image) {
  return ARMCreateReceiveRel32<ARM32Rel32Parser::AddrTraits_T24>(translator_,
                                                                 image.begin());
}

/******** DisassemblerElfAArch64 ********/

DisassemblerElfAArch64::DisassemblerElfAArch64() = default;

DisassemblerElfAArch64::~DisassemblerElfAArch64() = default;

std::vector<ReferenceGroup> DisassemblerElfAArch64::MakeReferenceGroups()
    const {
  // Initialized on first use.
  return {
      {{8, TypeTag(AArch64ReferenceType::kReloc),
        PoolTag(ARMReferencePool::kPoolReloc)},
       &Super::MakeFindRelocs,
       &Super::MakeReceiveRelocs},
      {{4, TypeTag(AArch64ReferenceType::kAbs32),
        PoolTag(ARMReferencePool::kPoolAbs32)},
       &Super::MakeFindAbs32,
       &Super::MakeReceiveAbs32},
      {{4, TypeTag(AArch64ReferenceType::kRel32_Immd14),
        PoolTag(ARMReferencePool::kPoolRel32)},
       &DisassemblerElfAArch64::MakeFindRel32Immd14,
       &DisassemblerElfAArch64::MakeReceiveRel32Immd14},
      {{4, TypeTag(AArch64ReferenceType::kRel32_Immd19),
        PoolTag(ARMReferencePool::kPoolRel32)},
       &DisassemblerElfAArch64::MakeFindRel32Immd19,
       &DisassemblerElfAArch64::MakeReceiveRel32Immd19},
      {{4, TypeTag(AArch64ReferenceType::kRel32_Immd26),
        PoolTag(ARMReferencePool::kPoolRel32)},
       &DisassemblerElfAArch64::MakeFindRel32Immd26,
       &DisassemblerElfAArch64::MakeReceiveRel32Immd26},
  };
}

uint32_t DisassemblerElfAArch64::GetRelType() const {
  // TODO(huangs): See if R_AARCH64_GLOB_DAT and R_AARCH64_JUMP_SLOT should
  // be used.
  return R_AARCH64_RELATIVE;
}

bool DisassemblerElfAArch64::ParseExecSection(
    const typename ELF64Traits::Elf_Shdr& section) {
  ConstBufferView region(image_.begin() + section.sh_offset, section.sh_size);
  Abs32GapFinder gap_finder(image_, region, abs32_locations_, 4);
  Rel32FinderAArch64 finder(image_, translator_);
  for (auto gap = gap_finder.GetNext(); gap.has_value();
       gap = gap_finder.GetNext()) {
    finder.Reset(gap.value());
    for (auto rel32 = finder.GetNext(); rel32.has_value();
         rel32 = finder.GetNext()) {
      // Reject illegal offsets for robustness.
      if (IsOffsetInExecSection(rel32->target_offset)) {
        finder.Accept();
        rel32_locations_table_[rel32->type].push_back(rel32->offset);
      }
    }
  }
  return true;
}

std::unique_ptr<ReferenceReader> DisassemblerElfAArch64::MakeFindRel32Immd14(
    offset_t lower,
    offset_t upper) {
  return ARMCreateFindRel32<AArch64Rel32Parser::AddrTraits_Immd14>(
      translator_, Super::image_.begin(),
      rel32_locations_table_[AArch64Rel32Parser::ADDR_IMMD14], lower, upper);
}

std::unique_ptr<ReferenceWriter> DisassemblerElfAArch64::MakeReceiveRel32Immd14(
    MutableBufferView image) {
  return ARMCreateReceiveRel32<AArch64Rel32Parser::AddrTraits_Immd14>(
      translator_, image.begin());
}

std::unique_ptr<ReferenceReader> DisassemblerElfAArch64::MakeFindRel32Immd19(
    offset_t lower,
    offset_t upper) {
  return ARMCreateFindRel32<AArch64Rel32Parser::AddrTraits_Immd19>(
      translator_, Super::image_.begin(),
      rel32_locations_table_[AArch64Rel32Parser::ADDR_IMMD19], lower, upper);
}

std::unique_ptr<ReferenceWriter> DisassemblerElfAArch64::MakeReceiveRel32Immd19(
    MutableBufferView image) {
  return ARMCreateReceiveRel32<AArch64Rel32Parser::AddrTraits_Immd19>(
      translator_, image.begin());
}

std::unique_ptr<ReferenceReader> DisassemblerElfAArch64::MakeFindRel32Immd26(
    offset_t lower,
    offset_t upper) {
  return ARMCreateFindRel32<AArch64Rel32Parser::AddrTraits_Immd26>(
      translator_, Super::image_.begin(),
      rel32_locations_table_[AArch64Rel32Parser::ADDR_IMMD26], lower, upper);
}

std::unique_ptr<ReferenceWriter> DisassemblerElfAArch64::MakeReceiveRel32Immd26(
    MutableBufferView image) {
  return ARMCreateReceiveRel32<AArch64Rel32Parser::AddrTraits_Immd26>(
      translator_, image.begin());
}

/******** Helper Functions ********/

ExecutableType QuickDetectElf(ConstBufferView image) {
  BufferSource source(image);
  if (source.size() < 16 || !source.CheckNextBytes({0x7F, 'E', 'L', 'F'}))
    return kExeTypeUnknown;

  uint8_t ei_class = source.read<uint8_t>(4);
  if (ei_class == 1) {  // 32-bit ELF.
    using Traits = ELF32Traits;
    auto* header = source.GetPointer<typename Traits::Elf_Ehdr>();
    if (!header || (header->e_type != ET_EXEC && header->e_type != ET_DYN) ||
        header->e_version != 1 ||
        header->e_shentsize != sizeof(typename Traits::Elf_Shdr)) {
      return kExeTypeUnknown;
    }
    if (header->e_machine == EM_386)
      return kExeTypeElfX86;
    if (header->e_machine == Traits::kARMMachineValue)
      return kExeTypeElfArm32;

  } else if (ei_class == 2) {  // 64-bit ELF.
    using Traits = ELF64Traits;
    auto* header = source.GetPointer<typename Traits::Elf_Ehdr>();
    if (!header || (header->e_type != ET_EXEC && header->e_type != ET_DYN) ||
        header->e_version != 1 ||
        header->e_shentsize != sizeof(typename Traits::Elf_Shdr)) {
      return kExeTypeUnknown;
    }
    if (header->e_machine == Traits::kARMMachineValue)
      return kExeTypeElfAArch64;
  }
  return kExeTypeUnknown;
}

}  // namespace zucchini
