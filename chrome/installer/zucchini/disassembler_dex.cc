// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/disassembler_dex.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <functional>
#include <iterator>
#include <set>
#include <unordered_map>
#include <utility>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "chrome/installer/zucchini/algorithm.h"
#include "chrome/installer/zucchini/buffer_source.h"
#include "chrome/installer/zucchini/buffer_view.h"

namespace zucchini {

namespace {

// Returns pointer to DEX Instruction data for |opcode|, or null if |opcode| is
// unknown. An internal initialize-on-first-use table is used for fast lookup.
const dex::Instruction* FindDalvikInstruction(uint8_t opcode) {
  static bool is_init = false;
  static const dex::Instruction* instruction_table[256];
  if (!is_init) {
    is_init = true;
    std::fill(std::begin(instruction_table), std::end(instruction_table),
              nullptr);
    for (const dex::Instruction& instr : dex::kByteCode) {
      std::fill(instruction_table + instr.opcode,
                instruction_table + instr.opcode + instr.variant, &instr);
    }
  }
  return instruction_table[opcode];
}

// Returns an iterator on instructions start associated with a CodeItem.
ConstBufferView::const_iterator CodeItemToInstructions(
    const dex::CodeItem* section) {
  return reinterpret_cast<ConstBufferView::const_iterator>(section) +
         sizeof(dex::CodeItem);
}

// Parse DEX bytecode, generating instructions.
class InstructionParser {
 public:
  struct Value {
    ConstBufferView::const_iterator location;
    const dex::Instruction* instr;  // null for unknown instructions.
  };

  InstructionParser() = default;

  explicit InstructionParser(const dex::CodeItem* section)
      : code_(CodeItemToInstructions(section),
              section->insns_size * sizeof(uint16_t)) {}

  bool operator()(Value* v);

 private:
  ConstBufferView code_;
  uint32_t payload_size_ = 0;
};

bool InstructionParser::operator()(Value* v) {
  // Skip "payload" found at end of |code_|, which should not be disassembled.
  if (code_.size() <= payload_size_)
    return false;

  uint8_t op = code_.read<uint8_t>(0);
  const dex::Instruction* instr = FindDalvikInstruction(op);
  ConstBufferView::const_iterator location = code_.begin();
  *v = {location, instr};

  // Skip unknown instructions. ODEX files might trigger this.
  if (instr == nullptr) {
    LOG(WARNING) << "Warning: Unknown Dalvik detected.";
    code_.remove_prefix(sizeof(uint16_t));
    return true;
  }

  if (code_.size() < instr->layout * sizeof(uint16_t))
    return false;
  if (instr->opcode == 0x26 || instr->opcode == 0x2B || instr->opcode == 0x2C) {
    // fill-array-data; packed-switch; sparse-switch.
    // Instruction with variable length data payload.
    int32_t delta = code_.read<int32_t>(2) * sizeof(uint16_t);
    if (delta < 0 || static_cast<uint32_t>(delta) >= code_.size())
      return false;
    // That associated data are assumed to lie at the end of the code segment
    // as payload,
    ConstBufferView::const_iterator payload = location + delta;
    payload_size_ =
        std::max(payload_size_, static_cast<uint32_t>(code_.end() - payload));
  }
  code_.remove_prefix(instr->layout * sizeof(uint16_t));
  return true;
}

// Generate references found in DEX bytecode. |filter_| is a callable used to
// filter instruction that contains references. |mapper_| is a callable used to
// obtain the target of a reference given its location.
template <class Filter, class Mapper>
class InstructionGenerator : public ReferenceReader {
 public:
  using SectionIterator = std::vector<const dex::CodeItem*>::const_iterator;

  InstructionGenerator(ConstBufferView image,
                       Filter&& filter,
                       Mapper&& mapper,
                       SectionIterator first_section,
                       SectionIterator last_section,
                       offset_t lo,
                       offset_t hi);

  base::Optional<Reference> GetNext() override;

 private:
  Filter filter_;
  Mapper mapper_;
  ConstBufferView::const_iterator file_start_;
  SectionIterator cur_section_;
  SectionIterator last_section_;
  ConstBufferView::const_iterator lo_;
  ConstBufferView::const_iterator hi_;
  InstructionParser parser_;
};

template <class Filter, class Mapper>
InstructionGenerator<Filter, Mapper>::InstructionGenerator(
    ConstBufferView image,
    Filter&& filter,
    Mapper&& mapper,
    SectionIterator first_section,
    SectionIterator last_section,
    offset_t lo,
    offset_t hi)
    : filter_(std::move(filter)),
      mapper_(std::move(mapper)),
      file_start_(image.begin()),
      last_section_(last_section),
      lo_(lo + file_start_),
      hi_(hi + file_start_) {
  auto comp = [](ConstBufferView::const_iterator it,
                 const dex::CodeItem* section) {
    return it < CodeItemToInstructions(section);
  };
  cur_section_ =
      std::upper_bound(first_section, last_section, file_start_ + lo, comp);
  if (cur_section_ != first_section)
    --cur_section_;
  parser_ = InstructionParser(*cur_section_);
}

template <class Filter, class Mapper>
base::Optional<Reference> InstructionGenerator<Filter, Mapper>::GetNext() {
  for (;;) {
    for (InstructionParser::Value v; parser_(&v);) {
      if (v.location >= hi_)
        return base::nullopt;
      if (v.instr == nullptr)
        continue;
      auto loc = filter_(v);
      if (loc == nullptr || loc < lo_)
        continue;
      if (loc >= hi_)
        return base::nullopt;
      offset_t location = loc - file_start_;
      return Reference{location, mapper_(location)};
    }
    ++cur_section_;
    if (cur_section_ == last_section_)
      return base::nullopt;
    parser_ = InstructionParser(*cur_section_);
  }
}

// Generate references found in DEX items lists. |mapper_| is a callable used to
// obtain a reference given an item in the items list.
template <class Item, class Mapper>
class ItemGenerator : public ReferenceReader {
 public:
  ItemGenerator(ConstBufferView image,
                Mapper&& mapper,
                const dex::MapItem* items,
                offset_t lo,
                offset_t hi);

  base::Optional<Reference> GetNext();

 private:
  offset_t Location() const {
    offset_t loc = items_->offset + idx_ * sizeof(Item);
    Reference ref = mapper_(reinterpret_cast<const Item*>(file_start_ + loc));
    return ref.location + loc;
  }

  Mapper mapper_;
  ConstBufferView::const_iterator file_start_;
  const dex::MapItem* items_;
  offset_t idx_ = 0;
  offset_t hi_;
};

template <class Item, class Mapper>
ItemGenerator<Item, Mapper>::ItemGenerator(ConstBufferView image,
                                           Mapper&& mapper,
                                           const dex::MapItem* items,
                                           offset_t lo,
                                           offset_t hi)
    : mapper_(std::move(mapper)),
      file_start_(image.begin()),
      items_(items),
      hi_(hi) {
  idx_ = (lo >= items_->offset) ? (lo - items_->offset) / sizeof(Item) : 0;
  while (idx_ < items_->size && Location() < lo) {
    ++idx_;
  }
}

template <class Item, class Mapper>
base::Optional<Reference> ItemGenerator<Item, Mapper>::GetNext() {
  if (idx_ >= items_->size)
    return base::nullopt;

  offset_t loc = items_->offset + idx_ * sizeof(Item);
  Reference ref = mapper_(reinterpret_cast<const Item*>(file_start_ + loc));
  ref.location += loc;
  if (ref.location >= hi_)
    return base::nullopt;
  ++idx_;
  return ref;
}

}  // namespace

DisassemblerDex::DisassemblerDex() = default;
DisassemblerDex::~DisassemblerDex() = default;

// static.
bool DisassemblerDex::CheckMagic(const dex::HeaderItem* header, int* version) {
  DCHECK_NE(version, nullptr);
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
bool DisassemblerDex::QuickDetect(ConstBufferView image) {
  BufferSource source(image);

  if (!source.CheckNextBytes({'d', 'e', 'x', '\n'}))
    return false;

  const dex::HeaderItem* header = source.GetPointer<dex::HeaderItem>();
  if (!header)  // Rewind. Magic is part of header!
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
  source = std::move(BufferSource(image).Skip(header->map_off));
  if (!source.GetValue(&list_size) || list_size > dex::kMaxItemListSize ||
      !source.GetArray<dex::MapItem>(list_size)) {
    return false;
  }

  return true;
}

std::unique_ptr<ReferenceReader> DisassemblerDex::MakeFindRel32(offset_t lo,
                                                                offset_t hi) {
  auto filter = [](const InstructionParser::Value& value)
      -> ConstBufferView::const_iterator {
    if (value.instr->format == dex::FormatId::t &&
        (value.instr->opcode == 0x26 || value.instr->opcode == 0x2A ||
         value.instr->opcode == 0x2B || value.instr->opcode == 0x2C)) {
      // fill-array-data; goto/32; packed-switch; sparse-switch.
      return value.location + 2;
    }
    return nullptr;
  };
  auto mapper = [this](offset_t loc) {
    return offset_t(loc + image_.read<int32_t>(loc) * sizeof(uint16_t));
  };
  return base::MakeUnique<
      InstructionGenerator<decltype(filter), decltype(mapper)>>(
      image_, std::move(filter), std::move(mapper), code_item_elements_.begin(),
      code_item_elements_.end(), lo, hi);
}

std::unique_ptr<ReferenceReader> DisassemblerDex::MakeFindRel16(offset_t lo,
                                                                offset_t hi) {
  auto filter = [](const InstructionParser::Value& value)
      -> ConstBufferView::const_iterator {
    if (value.instr->format == dex::FormatId::t &&
        (value.instr->opcode == 0x29 || value.instr->opcode == 0x32 ||
         value.instr->opcode == 0x38)) {
      // fill-array-data; goto/32; packed-switch; sparse-switch.
      return value.location + 2;
    }
    return nullptr;
  };
  auto mapper = [this](offset_t loc) {
    return offset_t(loc + image_.read<int16_t>(loc) * sizeof(uint16_t));
  };
  return base::MakeUnique<
      InstructionGenerator<decltype(filter), decltype(mapper)>>(
      image_, std::move(filter), std::move(mapper), code_item_elements_.begin(),
      code_item_elements_.end(), lo, hi);
}

std::unique_ptr<ReferenceReader> DisassemblerDex::MakeFindByteCodeToMethod16(
    offset_t lo,
    offset_t hi) {
  auto filter = [](const InstructionParser::Value& value)
      -> ConstBufferView::const_iterator {
    if (value.instr->format == dex::FormatId::c &&
        (value.instr->opcode == 0x6E || value.instr->opcode == 0x74)) {
      // fill-array-data; goto/32; packed-switch; sparse-switch.
      return value.location + 2;
    }
    return nullptr;
  };
  auto mapper = [this](offset_t loc) {
    uint16_t idx = image_.read<uint16_t>(loc);
    DCHECK_LT(idx, method_items_->size);
    return offset_t(method_items_->offset + idx * sizeof(dex::MethodIdItem));
  };
  return base::MakeUnique<
      InstructionGenerator<decltype(filter), decltype(mapper)>>(
      image_, std::move(filter), std::move(mapper), code_item_elements_.begin(),
      code_item_elements_.end(), lo, hi);
}

std::unique_ptr<ReferenceReader> DisassemblerDex::MakeFindByteCodeToString16(
    offset_t lo,
    offset_t hi) {
  auto filter = [](const InstructionParser::Value& value)
      -> ConstBufferView::const_iterator {
    if (value.instr->format == dex::FormatId::c &&
        (value.instr->opcode == 0x1A)) {
      // fill-array-data; goto/32; packed-switch; sparse-switch.
      return value.location + 2;
    }
    return nullptr;
  };
  auto mapper = [this](offset_t loc) {
    uint16_t idx = image_.read<uint16_t>(loc);
    DCHECK_LT(idx, string_items_->size);
    return offset_t(string_items_->offset + idx * sizeof(dex::StringIdItem));
  };
  return base::MakeUnique<
      InstructionGenerator<decltype(filter), decltype(mapper)>>(
      image_, std::move(filter), std::move(mapper), code_item_elements_.begin(),
      code_item_elements_.end(), lo, hi);
}

std::unique_ptr<ReferenceReader> DisassemblerDex::MakeFindByteCodeToString32(
    offset_t lo,
    offset_t hi) {
  auto filter = [](const InstructionParser::Value& value)
      -> ConstBufferView::const_iterator {
    if (value.instr->format == dex::FormatId::c &&
        (value.instr->opcode == 0x1B)) {
      // fill-array-data; goto/32; packed-switch; sparse-switch.
      return value.location + 2;
    }
    return nullptr;
  };
  auto mapper = [this](offset_t loc) {
    uint32_t idx = image_.read<uint32_t>(loc);
    DCHECK_LT(idx, string_items_->size);
    return offset_t(string_items_->offset + idx * sizeof(dex::StringIdItem));
  };
  return base::MakeUnique<
      InstructionGenerator<decltype(filter), decltype(mapper)>>(
      image_, std::move(filter), std::move(mapper), code_item_elements_.begin(),
      code_item_elements_.end(), lo, hi);
}

std::unique_ptr<ReferenceReader> DisassemblerDex::MakeFindByteCodeToType16(
    offset_t lo,
    offset_t hi) {
  auto filter = [](const InstructionParser::Value& value)
      -> ConstBufferView::const_iterator {
    if (value.instr->format == dex::FormatId::c &&
        (value.instr->opcode == 0x1C || value.instr->opcode == 0x1F ||
         value.instr->opcode == 0x20 || value.instr->opcode == 0x22 ||
         value.instr->opcode == 0x23 || value.instr->opcode == 0x24 ||
         value.instr->opcode == 0x25)) {
      // fill-array-data; goto/32; packed-switch; sparse-switch.
      return value.location + 2;
    }
    return nullptr;
  };
  auto mapper = [this](offset_t loc) {
    uint16_t idx = image_.read<uint16_t>(loc);
    DCHECK_LT(idx, type_items_->size);
    return offset_t(type_items_->offset + idx * sizeof(dex::TypeIdItem));
  };
  return base::MakeUnique<
      InstructionGenerator<decltype(filter), decltype(mapper)>>(
      image_, std::move(filter), std::move(mapper), code_item_elements_.begin(),
      code_item_elements_.end(), lo, hi);
}

std::unique_ptr<ReferenceReader> DisassemblerDex::MakeFindByteCodeToField16(
    offset_t lo,
    offset_t hi) {
  auto filter = [](const InstructionParser::Value& value)
      -> ConstBufferView::const_iterator {
    if (value.instr->format == dex::FormatId::c &&
        (value.instr->opcode == 0x52 || value.instr->opcode == 0x60)) {
      // fill-array-data; goto/32; packed-switch; sparse-switch.
      return value.location + 2;
    }
    return nullptr;
  };
  auto mapper = [this](offset_t loc) {
    uint16_t idx = image_.read<uint16_t>(loc);
    DCHECK_LT(idx, field_items_->size);
    return offset_t(field_items_->offset + idx * sizeof(dex::FieldIdItem));
  };
  return base::MakeUnique<
      InstructionGenerator<decltype(filter), decltype(mapper)>>(
      image_, std::move(filter), std::move(mapper), code_item_elements_.begin(),
      code_item_elements_.end(), lo, hi);
}

std::unique_ptr<ReferenceReader> DisassemblerDex::MakeFindStringToData(
    offset_t lo,
    offset_t hi) {
  auto mapper = [](const dex::StringIdItem* item) -> Reference {
    return {offsetof(dex::StringIdItem, string_data_off),
            item->string_data_off};
  };
  return base::MakeUnique<ItemGenerator<dex::StringIdItem, decltype(mapper)>>(
      image_, std::move(mapper), string_items_, lo, hi);
}

std::unique_ptr<ReferenceReader> DisassemblerDex::MakeFindFieldToName(
    offset_t lo,
    offset_t hi) {
  auto mapper = [this](const dex::FieldIdItem* item) -> Reference {
    DCHECK_LT(item->name_idx, string_items_->size);
    offset_t target = offset_t(string_items_->offset +
                               item->name_idx * sizeof(dex::StringIdItem));
    return {offsetof(dex::FieldIdItem, name_idx), target};
  };
  return base::MakeUnique<ItemGenerator<dex::FieldIdItem, decltype(mapper)>>(
      image_, std::move(mapper), field_items_, lo, hi);
}

std::unique_ptr<ReferenceReader> DisassemblerDex::MakeFindFieldToClass(
    offset_t lo,
    offset_t hi) {
  auto mapper = [this](const dex::FieldIdItem* item) -> Reference {
    DCHECK_LT(item->class_idx, type_items_->size);
    offset_t target = offset_t(type_items_->offset +
                               item->class_idx * sizeof(dex::TypeIdItem));
    return {offsetof(dex::FieldIdItem, class_idx), target};
  };
  return base::MakeUnique<ItemGenerator<dex::FieldIdItem, decltype(mapper)>>(
      image_, std::move(mapper), field_items_, lo, hi);
}

std::unique_ptr<ReferenceReader> DisassemblerDex::MakeFindFieldToType(
    offset_t lo,
    offset_t hi) {
  auto mapper = [this](const dex::FieldIdItem* item) -> Reference {
    DCHECK_LT(item->type_idx, type_items_->size);
    offset_t target = offset_t(type_items_->offset +
                               item->type_idx * sizeof(dex::TypeIdItem));
    return {offsetof(dex::FieldIdItem, type_idx), target};
  };
  return base::MakeUnique<ItemGenerator<dex::FieldIdItem, decltype(mapper)>>(
      image_, std::move(mapper), field_items_, lo, hi);
}

std::unique_ptr<ReferenceWriter> DisassemblerDex::MakeReceiveRel32(
    MutableBufferView image) {
  struct ReferenceWriterImpl : public ReferenceWriter {
    explicit ReferenceWriterImpl(MutableBufferView image_in)
        : image(image_in) {}
    void PutNext(Reference ref) override {
      int32_t offset = (ref.target - ref.location) >> 1;
      image.write<int32_t>(ref.location, offset);
    }
    MutableBufferView image;
  };
  return base::MakeUnique<ReferenceWriterImpl>(image);
}

std::unique_ptr<ReferenceWriter> DisassemblerDex::MakeReceiveRel16(
    MutableBufferView image) {
  struct ReferenceWriterImpl : public ReferenceWriter {
    explicit ReferenceWriterImpl(MutableBufferView image_in)
        : image(image_in) {}
    void PutNext(Reference ref) override {
      int16_t offset = (ref.target - ref.location) >> 1;
      image.write<int16_t>(ref.location, offset);
    }
    MutableBufferView image;
  };
  return base::MakeUnique<ReferenceWriterImpl>(image);
}

std::unique_ptr<ReferenceWriter> DisassemblerDex::MakeReceiveMethodIdx16(
    MutableBufferView image) {
  struct ReferenceWriterImpl : public ReferenceWriter {
    ReferenceWriterImpl(const DisassemblerDex* self_in,
                        MutableBufferView image_in)
        : self(self_in), image(image_in) {}
    void PutNext(Reference ref) override {
      uint16_t idx = uint16_t((ref.target - self->method_items_->offset) /
                              sizeof(dex::MethodIdItem));
      image.write<uint16_t>(ref.location, idx);
    }
    const DisassemblerDex* self;
    MutableBufferView image;
  };
  return base::MakeUnique<ReferenceWriterImpl>(this, image);
}

std::unique_ptr<ReferenceWriter> DisassemblerDex::MakeReceiveStringIdx16(
    MutableBufferView image) {
  struct ReferenceWriterImpl : public ReferenceWriter {
    ReferenceWriterImpl(const DisassemblerDex* self_in,
                        MutableBufferView image_in)
        : self(self_in), image(image_in) {}
    void PutNext(Reference ref) override {
      uint16_t idx = uint16_t((ref.target - self->string_items_->offset) /
                              sizeof(dex::StringIdItem));
      image.write<uint16_t>(ref.location, idx);
    }
    const DisassemblerDex* self;
    MutableBufferView image;
  };
  return base::MakeUnique<ReferenceWriterImpl>(this, image);
}

std::unique_ptr<ReferenceWriter> DisassemblerDex::MakeReceiveStringIdx32(
    MutableBufferView image) {
  struct ReferenceWriterImpl : public ReferenceWriter {
    ReferenceWriterImpl(const DisassemblerDex* self_in,
                        MutableBufferView image_in)
        : self(self_in), image(image_in) {}
    void PutNext(Reference ref) override {
      uint32_t idx = uint32_t((ref.target - self->string_items_->offset) /
                              sizeof(dex::StringIdItem));
      image.write<uint32_t>(ref.location, idx);
    }
    const DisassemblerDex* self;
    MutableBufferView image;
  };
  return base::MakeUnique<ReferenceWriterImpl>(this, image);
}

std::unique_ptr<ReferenceWriter> DisassemblerDex::MakeReceiveTypeIdx16(
    MutableBufferView image) {
  struct ReferenceWriterImpl : public ReferenceWriter {
    ReferenceWriterImpl(const DisassemblerDex* self_in,
                        MutableBufferView image_in)
        : self(self_in), image(image_in) {}
    void PutNext(Reference ref) override {
      uint16_t idx = uint16_t((ref.target - self->type_items_->offset) /
                              sizeof(dex::TypeIdItem));
      image.write<uint16_t>(ref.location, idx);
    }
    const DisassemblerDex* self;
    MutableBufferView image;
  };
  return base::MakeUnique<ReferenceWriterImpl>(this, image);
}

std::unique_ptr<ReferenceWriter> DisassemblerDex::MakeReceiveFieldIdx16(
    MutableBufferView image) {
  struct ReferenceWriterImpl : public ReferenceWriter {
    ReferenceWriterImpl(const DisassemblerDex* self_in,
                        MutableBufferView image_in)
        : self(self_in), image(image_in) {}
    void PutNext(Reference ref) override {
      uint16_t idx = uint16_t((ref.target - self->field_items_->offset) /
                              sizeof(dex::FieldIdItem));
      image.write<uint16_t>(ref.location, idx);
    }
    const DisassemblerDex* self;
    MutableBufferView image;
  };
  return base::MakeUnique<ReferenceWriterImpl>(this, image);
}

std::unique_ptr<ReferenceWriter> DisassemblerDex::MakeReceiveAbs32(
    MutableBufferView image) {
  struct ReferenceWriterImpl : public ReferenceWriter {
    explicit ReferenceWriterImpl(MutableBufferView image_in)
        : image(image_in) {}
    void PutNext(Reference ref) override {
      image.write<uint32_t>(ref.location, ref.target);
    }
    MutableBufferView image;
  };
  return base::MakeUnique<ReferenceWriterImpl>(image);
}

ExecutableType DisassemblerDex::GetExeType() const {
  return kExeTypeDex;
}

std::string DisassemblerDex::GetExeTypeString() const {
  constexpr int kBufSize = 32;
  char buf[kBufSize] = {0};
  snprintf(buf, kBufSize, "DEX (version %d)", dex_version_);
  return buf;
}

std::vector<ReferenceGroup> DisassemblerDex::MakeReferenceGroups() const {
  // Initialized on first use.
  return {
      {{4, TypeTag(kStringToData), PoolTag(kStringData)},
       &DisassemblerDex::MakeFindStringToData,
       &DisassemblerDex::MakeReceiveAbs32},
      {{2, TypeTag(kRel16), PoolTag(kRel)},
       &DisassemblerDex::MakeFindRel16,
       &DisassemblerDex::MakeReceiveRel16},
      {{4, TypeTag(kRel32), PoolTag(kRel)},
       &DisassemblerDex::MakeFindRel32,
       &DisassemblerDex::MakeReceiveRel32},
      {{2, TypeTag(kByteCodeToMethod16), PoolTag(kMethodIdx)},
       &DisassemblerDex::MakeFindByteCodeToMethod16,
       &DisassemblerDex::MakeReceiveMethodIdx16},
      {{2, TypeTag(kByteCodeToString16), PoolTag(kStringIdx)},
       &DisassemblerDex::MakeFindByteCodeToString16,
       &DisassemblerDex::MakeReceiveStringIdx16},
      {{4, TypeTag(kByteCodeToString32), PoolTag(kStringIdx)},
       &DisassemblerDex::MakeFindByteCodeToString32,
       &DisassemblerDex::MakeReceiveStringIdx32},
      {{4, TypeTag(kFieldToName), PoolTag(kStringIdx)},
       &DisassemblerDex::MakeFindFieldToName,
       &DisassemblerDex::MakeReceiveStringIdx32},
      {{2, TypeTag(kByteCodeToType16), PoolTag(kTypeIdx)},
       &DisassemblerDex::MakeFindByteCodeToType16,
       &DisassemblerDex::MakeReceiveTypeIdx16},
      {{2, TypeTag(kFieldToClass), PoolTag(kTypeIdx)},
       &DisassemblerDex::MakeFindFieldToClass,
       &DisassemblerDex::MakeReceiveTypeIdx16},
      {{2, TypeTag(kFieldToType), PoolTag(kTypeIdx)},
       &DisassemblerDex::MakeFindFieldToType,
       &DisassemblerDex::MakeReceiveTypeIdx16},
      {{2, TypeTag(kByteCodeToField16), PoolTag(kFieldIdx)},
       &DisassemblerDex::MakeFindByteCodeToField16,
       &DisassemblerDex::MakeReceiveFieldIdx16},
  };
}

bool DisassemblerDex::Parse(ConstBufferView image) {
  image_ = image;
  return ParseHeader();
}

bool DisassemblerDex::ParseHeader() {
  BufferSource source(image_);

  if (!source.CheckNextBytes({'d', 'e', 'x', '\n'}))
    return false;

  header_ = source.GetPointer<dex::HeaderItem>();
  if (!header_)  // Rewind. Magic is part of header!
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
  image_.shrink(header_->file_size);

  if (header_->map_off < sizeof(dex::HeaderItem))
    return false;

  // Read MapList: This is not a fixed-size array. Read components separately.
  DCHECK_EQ(offsetof(dex::MapList, list), sizeof(decltype(dex::MapList::size)));
  decltype(dex::MapList::size) list_size = 0;
  source = std::move(BufferSource(image_).Skip(header_->map_off));
  if (!source.GetValue(&list_size) || list_size > dex::kMaxItemListSize)
    return false;
  auto* item_list = source.GetArray<dex::MapItem>(list_size);
  if (!item_list)
    return false;

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
  if (!image_.covers_array(offset, code_items_->size, sizeof(dex::CodeItem)))
    return false;
  code_item_elements_.reserve(code_items_->size);

  for (size_t i = 0; i < code_items_->size; ++i) {
    offset = ceil(offset, offset_t(4));  // 4-byte alignment.
    if (!image_.covers({offset, sizeof(dex::CodeItem)}))
      return false;
    const dex::CodeItem* code_item =
        reinterpret_cast<const dex::CodeItem*>(image_.begin() + offset);
    code_item_elements_.push_back(code_item);

    offset += sizeof(dex::CodeItem);
    DCHECK_EQ(offset & 3, 0U);
    if (!image_.covers_array(offset, code_item->insns_size, sizeof(uint16_t)))
      return false;
    offset_t num_insns_bytes = code_item->insns_size * sizeof(uint16_t);
    offset = ceil(offset + num_insns_bytes, offset_t(4));

    if (code_item->tries_size > 0) {
      // |code_item| contains tries data.
      if (!image_.covers_array(offset, code_item->tries_size,
                               sizeof(dex::TryItem))) {
        return false;
      }
      // No padding needed.
      offset += code_item->tries_size * sizeof(dex::TryItem);

      ConstBufferView::const_iterator end = image_.end();
      ConstBufferView::const_iterator it =
          image_.begin() + offset;  // Still valid.
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
