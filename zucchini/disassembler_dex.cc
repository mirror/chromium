// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "zucchini/disassembler_dex.h"

#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <functional>
#include <iterator>
#include <set>
#include <unordered_map>
#include <utility>

#include "zucchini/ranges/algorithm.h"
#include "zucchini/region.h"

namespace zucchini {

namespace {

// Returns an iterator on instructions start associated with a CodeItem.
Region::const_iterator CodeItemToInstructions(const dex::CodeItem* section) {
  return reinterpret_cast<Region::const_iterator>(section) +
         sizeof(dex::CodeItem);
}

// Parse DEX bytecode, generating instructions.
class InstructionParser {
 public:
  struct Value {
    Region::const_iterator location;
    dex::Instruction instruction;
  };

  InstructionParser() = default;
  InstructionParser(Region::const_iterator first, Region::const_iterator last)
      : cur_(first), last_(last) {}
  explicit InstructionParser(const dex::CodeItem* section)
      : cur_(CodeItemToInstructions(section)),
        last_(cur_ + section->insns_size * sizeof(uint16_t)) {}

  bool operator()(Value* v);

 private:
  Region::const_iterator cur_ = nullptr;
  Region::const_iterator last_ = nullptr;
};

bool InstructionParser::operator()(Value* v) {
  if (cur_ >= last_)
    return false;
  auto* instr = ranges::upper_bound(dex::kByteCode, *cur_, ranges::less(),
                                    &dex::Instruction::opcode);
  assert(instr != std::begin(dex::kByteCode));
  --instr;
  assert(*cur_ >= instr->opcode && *cur_ < instr->opcode + instr->variant);
  Region::const_iterator location = cur_;
  if (instr->opcode == 0x26 || instr->opcode == 0x2b || instr->opcode == 0x2c) {
    // fill-array-data; packed-switch; sparse-switch.
    // Instruction with variable length data payload.
    Region::const_iterator target =
        cur_ + Region::at<int32_t>(cur_, 2) * sizeof(uint16_t);
    // We are assuming that associated data lies at the end of the code segment.
    // We shorten the code segment accordingly.
    assert(target > cur_);
    if (target < last_)
      last_ = target;
  }
  cur_ += instr->layout * sizeof(uint16_t);
  *v = {location, *instr};
  return true;
}

// Generate references found in DEX bytecode. |filter_| is a callable used to
// filter instruction that contains references. |mapper_| is a callable used to
// obtain the target of a reference given its location.
template <class Filter, class Mapper>
class InstructionGenerator {
 public:
  using SectionIterator = std::vector<const dex::CodeItem*>::const_iterator;

  InstructionGenerator(Region image,
                       Filter&& filter,
                       Mapper&& mapper,
                       SectionIterator first_section,
                       SectionIterator last_section,
                       offset_t lower_bound,
                       offset_t upper_bound);

  bool operator()(Reference* ref);

 private:
  Filter filter_;
  Mapper mapper_;
  Region::const_iterator file_start_;
  SectionIterator cur_section_;
  SectionIterator last_section_;
  Region::const_iterator lower_bound_;
  Region::const_iterator upper_bound_;
  InstructionParser parser_;
};

template <class Filter, class Mapper>
InstructionGenerator<Filter, Mapper>::InstructionGenerator(
    Region image,
    Filter&& filter,
    Mapper&& mapper,
    SectionIterator first_section,
    SectionIterator last_section,
    offset_t lower_bound,
    offset_t upper_bound)
    : filter_(std::move(filter)),
      mapper_(std::move(mapper)),
      file_start_(image.begin()),
      last_section_(last_section),
      lower_bound_(lower_bound + file_start_),
      upper_bound_(upper_bound + file_start_) {
  cur_section_ = ranges::upper_bound(first_section, last_section,
                                     file_start_ + lower_bound, ranges::less(),
                                     [](const dex::CodeItem* section) {
                                       return CodeItemToInstructions(section);
                                     });
  if (cur_section_ != first_section)
    --cur_section_;
  parser_ = InstructionParser(*cur_section_);
}

template <class Filter, class Mapper>
bool InstructionGenerator<Filter, Mapper>::operator()(Reference* ref) {
  for (;;) {
    for (InstructionParser::Value v; parser_(&v);) {
      if (v.location >= upper_bound_)
        return false;
      auto loc = filter_(v);
      if (loc == nullptr || loc < lower_bound_)
        continue;
      if (loc >= upper_bound_)
        return false;
      ref->location = loc - file_start_;
      ref->target = mapper_(ref->location);
      return true;
    }
    ++cur_section_;
    if (cur_section_ == last_section_)
      return false;
    parser_ = InstructionParser(*cur_section_);
  }
}

// Generate references found in DEX items lists. |mapper_| is a callable used to
// obtain a reference given an item in the items list.
template <class Item, class Mapper>
class ItemGenerator {
 public:
  ItemGenerator(Region image,
                Mapper&& mapper,
                const dex::MapItem* items,
                offset_t lower,
                offset_t upper);

  bool operator()(Reference* ref);

 private:
  offset_t Location() const {
    offset_t loc = items_->offset + idx_ * sizeof(Item);
    Reference ref = mapper_(Region::as<Item>(file_start_ + loc));
    return ref.location + loc;
  }

  Mapper mapper_;
  Region::const_iterator file_start_;
  const dex::MapItem* items_;
  offset_t idx_ = 0;
  offset_t upper_bound_;
};

template <class Item, class Mapper>
ItemGenerator<Item, Mapper>::ItemGenerator(Region image,
                                           Mapper&& mapper,
                                           const dex::MapItem* items,
                                           offset_t lower,
                                           offset_t upper)
    : mapper_(std::move(mapper)),
      file_start_(image.begin()),
      items_(items),
      upper_bound_(upper) {
  idx_ =
      (lower >= items_->offset) ? (lower - items_->offset) / sizeof(Item) : 0;
  while (idx_ < items_->size && Location() < lower) {
    ++idx_;
  }
}

template <class Item, class Mapper>
bool ItemGenerator<Item, Mapper>::operator()(Reference* ref) {
  if (idx_ >= items_->size)
    return false;

  offset_t loc = items_->offset + idx_ * sizeof(Item);
  *ref = mapper_(Region::as<Item>(file_start_ + loc));
  ref->location += loc;
  if (ref->location >= upper_bound_)
    return false;
  ++idx_;
  return true;
}

}  // namespace

DisassemblerDex::DisassemblerDex(Region image) : Disassembler(image) {}

DisassemblerDex::~DisassemblerDex() = default;

// static.
bool DisassemblerDex::CheckMagic(const dex::HeaderItem* header, int* version) {
  assert(version);
  if (header->magic[0] != 'd' || header->magic[1] != 'e' ||
      header->magic[2] != 'x' || header->magic[3] != '\n' ||
      header->magic[7] != '\0') {
    return false;
  }
  int temp = 0;
  for (int i = 4; i < 7; ++i) {
    if (!isdigit(header->magic[i]))
      return false;
    temp = temp * 10 + (header->magic[i] - '0');
  }
  *version = temp;
  return true;
}

// static.
bool DisassemblerDex::QuickDetect(Region image) {
  RegionStream rs(image);

  if (!rs.Eat('d', 'e', 'x', '\n'))
    return false;

  const dex::HeaderItem* header = nullptr;
  if (!rs.Seek(0) || !rs.ReadAs(&header))  // Rewind. Magic is part of header!
    return false;

  int dex_version = 0;
  if (!DisassemblerDex::CheckMagic(header, &dex_version))
    return false;
  if (dex_version != 35 && dex_version != 37)
    return false;

  if (header->file_size > image.size() ||
      header->file_size < sizeof(dex::HeaderItem)) {
    return false;
  }

  if (header->map_off < sizeof(dex::HeaderItem))
    return false;

  // See if MapList can be read.
  decltype(dex::MapList::size) list_size = 0;
  if (!rs.Seek(header->map_off) || !rs.ReadTo(&list_size) ||
      list_size > dex::kMaxItemListSize ||
      !rs.FitsArray<dex::MapItem>(list_size)) {
    return false;
  }

  return true;
}

ReferenceGenerator DisassemblerDex::FindRel32(offset_t lower, offset_t upper) {
  auto filter = [](const InstructionParser::Value& value) -> const_iterator {
    if (value.instruction.format == dex::FormatId::t &&
        (value.instruction.opcode == 0x26 || value.instruction.opcode == 0x2a ||
         value.instruction.opcode == 0x2b ||
         value.instruction.opcode == 0x2c)) {
      // fill-array-data; goto/32; packed-switch; sparse-switch.
      return value.location + 2;
    }
    return nullptr;
  };
  auto mapper = [this](offset_t loc) {
    return offset_t(loc + image_.at<int32_t>(loc) * sizeof(uint16_t));
  };
  return InstructionGenerator<decltype(filter), decltype(mapper)>(
      image_, std::move(filter), std::move(mapper), code_item_elements_.begin(),
      code_item_elements_.end(), lower, upper);
}

ReferenceGenerator DisassemblerDex::FindRel16(offset_t lower, offset_t upper) {
  auto filter = [](const InstructionParser::Value& value) -> const_iterator {
    if (value.instruction.format == dex::FormatId::t &&
        (value.instruction.opcode == 0x29 || value.instruction.opcode == 0x32 ||
         value.instruction.opcode == 0x38)) {
      // fill-array-data; goto/32; packed-switch; sparse-switch.
      return value.location + 2;
    }
    return nullptr;
  };
  auto mapper = [this](offset_t loc) {
    return offset_t(loc + image_.at<int16_t>(loc) * sizeof(uint16_t));
  };
  return InstructionGenerator<decltype(filter), decltype(mapper)>(
      image_, std::move(filter), std::move(mapper), code_item_elements_.begin(),
      code_item_elements_.end(), lower, upper);
}

ReferenceGenerator DisassemblerDex::FindByteCodeToMethod16(offset_t lower,
                                                           offset_t upper) {
  auto filter = [](const InstructionParser::Value& value) -> const_iterator {
    if (value.instruction.format == dex::FormatId::c &&
        (value.instruction.opcode == 0x6e ||
         value.instruction.opcode == 0x74)) {
      // fill-array-data; goto/32; packed-switch; sparse-switch.
      return value.location + 2;
    }
    return nullptr;
  };
  auto mapper = [this](offset_t loc) {
    uint16_t idx = image_.at<uint16_t>(loc);
    assert(idx < method_items_->size);
    return offset_t(method_items_->offset + idx * sizeof(dex::MethodIdItem));
  };
  return InstructionGenerator<decltype(filter), decltype(mapper)>(
      image_, std::move(filter), std::move(mapper), code_item_elements_.begin(),
      code_item_elements_.end(), lower, upper);
}

ReferenceGenerator DisassemblerDex::FindByteCodeToString16(offset_t lower,
                                                           offset_t upper) {
  auto filter = [](const InstructionParser::Value& value) -> const_iterator {
    if (value.instruction.format == dex::FormatId::c &&
        (value.instruction.opcode == 0x1a)) {
      // fill-array-data; goto/32; packed-switch; sparse-switch.
      return value.location + 2;
    }
    return nullptr;
  };
  auto mapper = [this](offset_t loc) {
    uint16_t idx = image_.at<uint16_t>(loc);
    assert(idx < string_items_->size);
    return offset_t(string_items_->offset + idx * sizeof(dex::StringIdItem));
  };
  return InstructionGenerator<decltype(filter), decltype(mapper)>(
      image_, std::move(filter), std::move(mapper), code_item_elements_.begin(),
      code_item_elements_.end(), lower, upper);
}

ReferenceGenerator DisassemblerDex::FindByteCodeToString32(offset_t lower,
                                                           offset_t upper) {
  auto filter = [](const InstructionParser::Value& value) -> const_iterator {
    if (value.instruction.format == dex::FormatId::c &&
        (value.instruction.opcode == 0x1b)) {
      // fill-array-data; goto/32; packed-switch; sparse-switch.
      return value.location + 2;
    }
    return nullptr;
  };
  auto mapper = [this](offset_t loc) {
    uint32_t idx = image_.at<uint32_t>(loc);
    assert(idx < string_items_->size);
    return offset_t(string_items_->offset + idx * sizeof(dex::StringIdItem));
  };
  return InstructionGenerator<decltype(filter), decltype(mapper)>(
      image_, std::move(filter), std::move(mapper), code_item_elements_.begin(),
      code_item_elements_.end(), lower, upper);
}

ReferenceGenerator DisassemblerDex::FindByteCodeToType16(offset_t lower,
                                                         offset_t upper) {
  auto filter = [](const InstructionParser::Value& value) -> const_iterator {
    if (value.instruction.format == dex::FormatId::c &&
        (value.instruction.opcode == 0x1c || value.instruction.opcode == 0x1f ||
         value.instruction.opcode == 0x20 || value.instruction.opcode == 0x22 ||
         value.instruction.opcode == 0x23 || value.instruction.opcode == 0x24 ||
         value.instruction.opcode == 0x25)) {
      // fill-array-data; goto/32; packed-switch; sparse-switch.
      return value.location + 2;
    }
    return nullptr;
  };
  auto mapper = [this](offset_t loc) {
    uint16_t idx = image_.at<uint16_t>(loc);
    assert(idx < type_items_->size);
    return offset_t(type_items_->offset + idx * sizeof(dex::TypeIdItem));
  };
  return InstructionGenerator<decltype(filter), decltype(mapper)>(
      image_, std::move(filter), std::move(mapper), code_item_elements_.begin(),
      code_item_elements_.end(), lower, upper);
}

ReferenceGenerator DisassemblerDex::FindByteCodeToField16(offset_t lower,
                                                          offset_t upper) {
  auto filter = [](const InstructionParser::Value& value) -> const_iterator {
    if (value.instruction.format == dex::FormatId::c &&
        (value.instruction.opcode == 0x52 ||
         value.instruction.opcode == 0x60)) {
      // fill-array-data; goto/32; packed-switch; sparse-switch.
      return value.location + 2;
    }
    return nullptr;
  };
  auto mapper = [this](offset_t loc) {
    uint16_t idx = image_.at<uint16_t>(loc);
    assert(idx < field_items_->size);
    return offset_t(field_items_->offset + idx * sizeof(dex::FieldIdItem));
  };
  return InstructionGenerator<decltype(filter), decltype(mapper)>(
      image_, std::move(filter), std::move(mapper), code_item_elements_.begin(),
      code_item_elements_.end(), lower, upper);
}

ReferenceGenerator DisassemblerDex::FindStringToData(offset_t lower,
                                                     offset_t upper) {
  auto mapper = [](const dex::StringIdItem* item) -> Reference {
    return {offsetof(dex::StringIdItem, string_data_off),
            item->string_data_off};
  };
  return ItemGenerator<dex::StringIdItem, decltype(mapper)>(
      image_, std::move(mapper), string_items_, lower, upper);
}

ReferenceGenerator DisassemblerDex::FindFieldToName(offset_t lower,
                                                    offset_t upper) {
  auto mapper = [this](const dex::FieldIdItem* item) -> Reference {
    assert(item->name_idx < string_items_->size);
    offset_t target = offset_t(string_items_->offset +
                               item->name_idx * sizeof(dex::StringIdItem));
    return {offsetof(dex::FieldIdItem, name_idx), target};
  };
  return ItemGenerator<dex::FieldIdItem, decltype(mapper)>(
      image_, std::move(mapper), field_items_, lower, upper);
}

ReferenceGenerator DisassemblerDex::FindFieldToClass(offset_t lower,
                                                     offset_t upper) {
  auto mapper = [this](const dex::FieldIdItem* item) -> Reference {
    assert(item->class_idx < type_items_->size);
    offset_t target = offset_t(type_items_->offset +
                               item->class_idx * sizeof(dex::TypeIdItem));
    return {offsetof(dex::FieldIdItem, class_idx), target};
  };
  return ItemGenerator<dex::FieldIdItem, decltype(mapper)>(
      image_, std::move(mapper), field_items_, lower, upper);
}

ReferenceGenerator DisassemblerDex::FindFieldToType(offset_t lower,
                                                    offset_t upper) {
  auto mapper = [this](const dex::FieldIdItem* item) -> Reference {
    assert(item->type_idx < type_items_->size);
    offset_t target = offset_t(type_items_->offset +
                               item->type_idx * sizeof(dex::TypeIdItem));
    return {offsetof(dex::FieldIdItem, type_idx), target};
  };
  return ItemGenerator<dex::FieldIdItem, decltype(mapper)>(
      image_, std::move(mapper), field_items_, lower, upper);
}

ReferenceReceptor DisassemblerDex::Rel32Receptor() {
  return [this](Reference ref) {
    int32_t offset = (ref.target - ref.location) >> 1;
    Region::at<int32_t>(image_.begin(), ref.location) = offset;
  };
}

ReferenceReceptor DisassemblerDex::Rel16Receptor() {
  return [this](Reference ref) {
    int16_t offset = (ref.target - ref.location) >> 1;
    image_.at<int16_t>(ref.location) = offset;
  };
}

ReferenceReceptor DisassemblerDex::MethodIdx16Receptor() {
  return [this](Reference ref) {
    uint16_t idx = uint16_t((ref.target - method_items_->offset) /
                            sizeof(dex::MethodIdItem));
    image_.at<uint16_t>(ref.location) = idx;
  };
}

ReferenceReceptor DisassemblerDex::StringIdx16Receptor() {
  return [this](Reference ref) {
    uint16_t idx = uint16_t((ref.target - string_items_->offset) /
                            sizeof(dex::StringIdItem));
    image_.at<uint16_t>(ref.location) = idx;
  };
}

ReferenceReceptor DisassemblerDex::StringIdx32Receptor() {
  return [this](Reference ref) {
    uint32_t idx = uint32_t((ref.target - string_items_->offset) /
                            sizeof(dex::StringIdItem));
    image_.at<uint32_t>(ref.location) = idx;
  };
}

ReferenceReceptor DisassemblerDex::TypeIdx16Receptor() {
  return [this](Reference ref) {
    uint16_t idx =
        uint16_t((ref.target - type_items_->offset) / sizeof(dex::TypeIdItem));
    image_.at<uint16_t>(ref.location) = idx;
  };
}

ReferenceReceptor DisassemblerDex::FieldIdx16Receptor() {
  return [this](Reference ref) {
    uint16_t idx = uint16_t((ref.target - field_items_->offset) /
                            sizeof(dex::FieldIdItem));
    image_.at<uint16_t>(ref.location) = idx;
  };
}

ReferenceReceptor DisassemblerDex::Abs32Receptor() {
  return
      [this](Reference ref) { image_.at<uint32_t>(ref.location) = ref.target; };
}

ExecutableType DisassemblerDex::GetExeType() const {
  return EXE_TYPE_DEX;
}

std::string DisassemblerDex::GetExeTypeString() const {
  constexpr int kBufSize = 32;
  char buf[kBufSize] = {0};
  snprintf(buf, kBufSize, "DEX (version %d)", dex_version_);
  return buf;
}

ReferenceTraits DisassemblerDex::GetReferenceTraits(uint8_t type) const {
  // Initialized on first use.
  static const std::unordered_map<uint8_t, ReferenceTraits> traits_map = {
      {kStringToData,
       {4, kStringToData, kStringData, &DisassemblerDex::FindStringToData,
        &DisassemblerDex::Abs32Receptor}},
      {kRel16,
       {2, kRel16, kRel, &DisassemblerDex::FindRel16,
        &DisassemblerDex::Rel16Receptor}},
      {kRel32,
       {4, kRel32, kRel, &DisassemblerDex::FindRel32,
        &DisassemblerDex::Rel32Receptor}},
      {kByteCodeToMethod16,
       {2, kByteCodeToMethod16, kMethodIdx,
        &DisassemblerDex::FindByteCodeToMethod16,
        &DisassemblerDex::MethodIdx16Receptor}},
      {kByteCodeToString16,
       {2, kByteCodeToString16, kStringIdx,
        &DisassemblerDex::FindByteCodeToString16,
        &DisassemblerDex::StringIdx16Receptor}},
      {kByteCodeToString32,
       {4, kByteCodeToString32, kStringIdx,
        &DisassemblerDex::FindByteCodeToString32,
        &DisassemblerDex::StringIdx32Receptor}},
      {kFieldToName,
       {4, kFieldToName, kStringIdx, &DisassemblerDex::FindFieldToName,
        &DisassemblerDex::StringIdx32Receptor}},
      {kByteCodeToType16,
       {2, kByteCodeToType16, kTypeIdx, &DisassemblerDex::FindByteCodeToType16,
        &DisassemblerDex::TypeIdx16Receptor}},
      {kFieldToClass,
       {2, kFieldToClass, kTypeIdx, &DisassemblerDex::FindFieldToClass,
        &DisassemblerDex::TypeIdx16Receptor}},
      {kFieldToType,
       {2, kFieldToType, kTypeIdx, &DisassemblerDex::FindFieldToType,
        &DisassemblerDex::TypeIdx16Receptor}},
      {kByteCodeToField16,
       {2, kByteCodeToField16, kFieldIdx,
        &DisassemblerDex::FindByteCodeToField16,
        &DisassemblerDex::FieldIdx16Receptor}},
  };
  return traits_map.at(type);
}

uint8_t DisassemblerDex::GetReferenceTraitsCount() const {
  return kTypeCount;
}

bool DisassemblerDex::Parse() {
  return ParseHeader();
}

bool DisassemblerDex::ParseHeader() {
  RegionStream rs(image_);

  if (!rs.Eat('d', 'e', 'x', '\n'))
    return false;

  if (!rs.Seek(0) || !rs.ReadAs(&header_))  // Rewind. Magic is part of header!
    return false;

  if (!DisassemblerDex::CheckMagic(header_, &dex_version_))
    return false;
  if (dex_version_ != 35 && dex_version_ != 37)
    return false;

  // DEX stores its file size, so use it to resize |image_| right away.
  if (header_->file_size > image_.size() ||
      header_->file_size < sizeof(dex::HeaderItem)) {
    return false;
  }
  image_.resize(header_->file_size);

  if (header_->map_off < sizeof(dex::HeaderItem))
    return false;

  // Read MapList: This is not a fixed-size array. Read components separately.
  assert(offsetof(dex::MapList, list) == sizeof(decltype(dex::MapList::size)));
  decltype(dex::MapList::size) list_size = 0;
  decltype(&dex::MapList::list[0]) item_list = nullptr;
  if (!rs.Seek(header_->map_off) || !rs.ReadTo(&list_size) ||
      list_size > dex::kMaxItemListSize ||
      !rs.ReadAsArray(&item_list, list_size)) {
    return false;
  }

  std::set<uint16_t> required_item_types = {
      dex::kTypeCodeItem, dex::kTypeMethodIdItem, dex::kTypeStringIdItem,
      dex::kTypeTypeIdItem, dex::kTypeFieldIdItem};
  for (offset_t i = 0; i < list_size; ++i) {
    const dex::MapItem* item = &item_list[i];
    if (item->offset >= image_.size())
      return false;
    if (!map_item_map_.insert(std::make_pair(item->type, item)).second)
      return false;  // A given type must appear at most once.
    required_item_types.erase(item->type);
  }
  if (!required_item_types.empty())
    return false;

  code_items_ = map_item_map_[dex::kTypeCodeItem];
  method_items_ = map_item_map_[dex::kTypeMethodIdItem];
  string_items_ = map_item_map_[dex::kTypeStringIdItem];
  type_items_ = map_item_map_[dex::kTypeTypeIdItem];
  field_items_ = map_item_map_[dex::kTypeFieldIdItem];

  // Go through the code item array to extract code sections.
  offset_t offset = code_items_->offset;
  // Sanity check for |code_items_->size|. We still need to check every element
  // during iteration, since code item actually has variable sizes.
  if (!CheckDataArrayFit(offset, code_items_->size, sizeof(dex::CodeItem),
                         image_.size())) {
    return false;
  }
  code_item_elements_.reserve(code_items_->size);

  for (size_t i = 0; i < code_items_->size; ++i) {
    offset = ceil(offset, offset_t(4));  // 4-byte alignment.
    if (!CheckDataFit(offset, sizeof(dex::CodeItem), image_.size()))
      return false;
    const dex::CodeItem* code_item = image_.as<dex::CodeItem>(offset);
    code_item_elements_.push_back(code_item);

    offset += sizeof(dex::CodeItem);
    assert((offset & 3) == 0);
    if (!CheckDataArrayFit(offset, sizeof(uint16_t), code_item->insns_size,
                           image_.size())) {
      return false;
    }
    offset_t num_insns_bytes = code_item->insns_size * sizeof(uint16_t);
    offset = ceil(offset + num_insns_bytes, offset_t(4));

    if (code_item->tries_size > 0) {
      // |code_item| contains tries data.
      if (!CheckDataArrayFit(offset, sizeof(dex::TryItem),
                             code_item->tries_size, image_.size())) {
        return false;
      }
      // No padding needed.
      offset += code_item->tries_size * sizeof(dex::TryItem);

      Region::const_iterator end = image_.end();
      Region::const_iterator it = image_.begin() + offset;  // Still valid.
      uint32_t encoded_catch_handler_list_size = 0;
      // Reading uleb128 or sleb128 numbers: To prevent buffer overrun, we
      // assume conservatively that each number will take maximal bytes. Since
      // |code_item| is unlikely to be the last section, this should not fail
      // for regular DEX files.
      if (end - it < dex::kMaxUleb128Size)
        return false;
      it = dex::DecodeUleb128(it, &encoded_catch_handler_list_size);

      for (size_t k = 0; k < encoded_catch_handler_list_size; ++k) {
        int32_t encoded_catch_handler_size = 0;
        if (end - it < dex::kMaxUleb128Size)
          return false;
        it = dex::DecodeSleb128(it, &encoded_catch_handler_size);

        size_t j_end = std::abs(encoded_catch_handler_size);
        for (size_t j = 0; j < j_end; ++j) {
          if (end - it < dex::kMaxUleb128Size)
            return false;
          it = dex::SkipLeb128(it);
          if (end - it < dex::kMaxUleb128Size)
            return false;
          it = dex::SkipLeb128(it);
        }
        if (encoded_catch_handler_size <= 0) {
          if (end - it < dex::kMaxUleb128Size)
            return false;
          it = dex::SkipLeb128(it);
        }
      }
      offset = it - image_.begin();
    }
  }

  return true;
}

}  // namespace zucchini
