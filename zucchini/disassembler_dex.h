// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZUCCHINI_DISASSEMBLER_DEX_H_
#define ZUCCHINI_DISASSEMBLER_DEX_H_

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "base/macros.h"
#include "zucchini/disassembler.h"
#include "zucchini/image_utils.h"
#include "zucchini/type_dex.h"

namespace zucchini {

class DisassemblerDex : public Disassembler {
 public:
  enum ReferencePool : uint8_t {
    kStringData,
    kRel,
    kMethodIdx,
    kStringIdx,
    kTypeIdx,
    kFieldIdx,
    kProtoIdx,
    kTypeList,
  };

  // We assume that types are grouped by pools.
  enum ReferenceType : uint8_t {
    kStringToData,  // kStringData

    kRel16,  // kRel
    kRel32,

    kByteCodeToMethod16,  // kMethodIdx

    kByteCodeToString16,  // kStringIdx
    kByteCodeToString32,
    kFieldToName,

    kByteCodeToType16,  // kTypeIdx
    kFieldToClass,
    kFieldToType,

    kByteCodeToField16,  // kFieldIdx

    // TOTO(etiennep) : Extract the following kind of pointers.
    // kMethodToClass,
    // kMethodToProto,
    // kMethodToName,
    // kProtoToShorty,
    // kProtoToReturn,
    // kProtoToParams,
    // kTypeListToType,
    // kClassDefToClass,
    // kClassDefToSuperClass,
    // kClassDefToInterfaces,
    kTypeCount
  };

  explicit DisassemblerDex(Region image);
  ~DisassemblerDex() override;

  // Checks DEX magic header. If successful, returns true and updates
  // |*version|. Otherwise returns false.
  static bool CheckMagic(const dex::HeaderItem* header, int* version);

  // Applies quick checks to determine if |image| *may* point to the start of an
  // executable. Returns true on success.
  static bool QuickDetect(Region image);

  ReferenceGenerator FindRel32(offset_t lower, offset_t upper);
  ReferenceGenerator FindRel16(offset_t lower, offset_t upper);
  ReferenceGenerator FindByteCodeToMethod16(offset_t lower, offset_t upper);
  ReferenceGenerator FindByteCodeToString16(offset_t lower, offset_t upper);
  ReferenceGenerator FindByteCodeToString32(offset_t lower, offset_t upper);
  ReferenceGenerator FindByteCodeToType16(offset_t lower, offset_t upper);
  ReferenceGenerator FindByteCodeToField16(offset_t lower, offset_t upper);
  ReferenceGenerator FindStringToData(offset_t lower, offset_t upper);
  ReferenceGenerator FindFieldToName(offset_t lower, offset_t upper);
  ReferenceGenerator FindFieldToClass(offset_t lower, offset_t upper);
  ReferenceGenerator FindFieldToType(offset_t lower, offset_t upper);

  ReferenceReceptor Rel32Receptor();
  ReferenceReceptor Rel16Receptor();
  ReferenceReceptor MethodIdx16Receptor();
  ReferenceReceptor StringIdx16Receptor();
  ReferenceReceptor StringIdx32Receptor();
  ReferenceReceptor TypeIdx16Receptor();
  ReferenceReceptor FieldIdx16Receptor();
  ReferenceReceptor Abs32Receptor();

  // Disassembler:
  ExecutableType GetExeType() const override;
  std::string GetExeTypeString() const override;
  ReferenceTraits GetReferenceTraits(uint8_t type) const override;
  uint8_t GetReferenceTraitsCount() const override;
  bool Parse() override;

 private:
  using MapItemMap = std::map<uint16_t, const dex::MapItem*>;

  bool ParseHeader();

  const dex::HeaderItem* header_;
  int dex_version_ = 0;
  MapItemMap map_item_map_;
  const dex::MapItem* code_items_ = nullptr;
  const dex::MapItem* method_items_ = nullptr;
  const dex::MapItem* string_items_ = nullptr;
  const dex::MapItem* type_items_ = nullptr;
  const dex::MapItem* field_items_ = nullptr;
  std::vector<const dex::CodeItem*> code_item_elements_;
};

}  // namespace zucchini

#endif  // ZUCCHINI_DISASSEMBLER_DEX_H_
