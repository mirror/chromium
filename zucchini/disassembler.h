// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZUCCHINI_DISASSEMBLER_H_
#define ZUCCHINI_DISASSEMBLER_H_

#include <cstdint>
#include <functional>
#include <string>

#include "zucchini/image_utils.h"
#include "zucchini/ranges/functional.h"
#include "zucchini/region.h"

namespace zucchini {

class Disassembler;

// What type of executable is something.
enum ExecutableType : uint32_t {
  EXE_TYPE_UNKNOWN = UINT32_MAX,
  EXE_TYPE_NO_OP = 0,
  EXE_TYPE_WIN32_X86 = 1,
  EXE_TYPE_WIN32_X64 = 2,
  EXE_TYPE_ELF_X86 = 3,
  EXE_TYPE_ELF_ARM32 = 4,
  EXE_TYPE_ELF_AARCH64 = 5,
  EXE_TYPE_DEX = 6,
};

// Specification of references in an image file. |type| enumerates different
// ways to store references, and affects parsing and writing. |pool| enumerates
// semantics of a reference, and is used for patch generation. Multiple |type|
// can belong to a single |pool|; so |pool| is a coarsening of |type|. For
// example, on some architectures, conditional jump targets can be encoded in
// 1-byte or 2-byte forms, and these represent different |type|s. However, these
// references target code addresses, and having them in the same |pool| reduces
// redundancy.
struct ReferenceTraitsBasic {
  ReferenceTraitsBasic() = default;

  ReferenceTraitsBasic(offset_t w, uint8_t t, uint8_t p)
      : width(w), type(t), pool(p) {}

  offset_t width = 0;  // Number of bytes to store reference in the image file.
  uint8_t type = kNoRefType;
  uint8_t pool = kNoRefType;
};

// Augments ReferenceTraitsBasic with data access functors.
class ReferenceTraits : public ReferenceTraitsBasic {
 public:
  using FindFn = ReferenceGenerator (Disassembler::*)(offset_t lower,
                                                      offset_t upper);
  using ReceiveFn = ReferenceReceptor (Disassembler::*)();

  ReferenceTraits() = default;
  // F and R don't have to be identical to FindFn and ReceiveFn, but they must
  // be convertible. As a result, they can be pointer to member function of a
  // derived Disassembler.
  template <class F, class R>
  ReferenceTraits(offset_t w, uint8_t t, uint8_t p, F f, R r)
      : ReferenceTraitsBasic(w, t, p),
        find_(static_cast<FindFn>(f)),
        receive_(static_cast<ReceiveFn>(r)) {}
  // Special case where type and pool are not distinguished.
  template <class F, class R>
  ReferenceTraits(offset_t w, uint8_t t, F f, R r)
      : ReferenceTraits(w, t, t, f, r) {}

 protected:
  friend class ReferenceGroup;
  FindFn find_ = nullptr;
  ReceiveFn receive_ = nullptr;
};

// Lightweight class that decribes references of a particular type and provides
// an interface to access and write to them.
class ReferenceGroup {
 public:
  ReferenceGroup(const ReferenceTraits& traits, Disassembler* disasm)
      : traits_(traits), disasm_(disasm) {}

  ReferenceGenerator Find() const;
  ReferenceGenerator Find(offset_t lower, offset_t upper) const;
  ReferenceReceptor Receive() const;
  inline const ReferenceTraits& Traits() const { return traits_; }
  inline offset_t Width() const { return traits_.width; }
  inline uint8_t Type() const { return traits_.type; }
  inline uint8_t Pool() const { return traits_.pool; }

 private:
  ReferenceTraits traits_;
  Disassembler* disasm_;
};

// Lightweight view that can be iterated on to access all ReferenceGroups.
class GroupView {
 public:
  class Cursor : public ranges::BaseCursor<Cursor> {
   public:
    using iterator_category = std::random_access_iterator_tag;

    Cursor(Disassembler* disasm, std::ptrdiff_t idx)
        : disasm_(disasm), idx_(idx) {}
    ReferenceGroup get() const;
    ReferenceGroup at(std::ptrdiff_t n) const;

    void advance(std::ptrdiff_t n) { idx_ += n; }
    std::ptrdiff_t distance(Cursor that) const { return idx_ - that.idx_; }

   private:
    Disassembler* disasm_ = nullptr;
    std::ptrdiff_t idx_;
  };

  using iterator = ranges::IteratorFacade<Cursor>;

  GroupView(Disassembler* disasm, uint8_t group_count)
      : disasm_(disasm), group_count_(group_count) {}

  iterator begin() const { return {Cursor(disasm_, 0)}; }
  iterator end() const { return {Cursor(disasm_, group_count_)}; }

 private:
  Disassembler* disasm_;
  uint8_t group_count_;
};

using GroupRange = ranges::RngFacade<GroupView>;

// Provides an interface to find and write references in an image file.
class Disassembler : public AddressTranslator {
 public:
  using iterator = uint8_t*;
  using const_iterator = const uint8_t*;

  ~Disassembler() override = default;

  // AddressTranslator interfaces.
  RVAToOffsetTranslator GetRVAToOffsetTranslator() const override;
  OffsetToRVATranslator GetOffsetToRVATranslator() const override;

  // Returns the type of exectuable handled by the disassembler.
  virtual ExecutableType GetExeType() const = 0;

  // Returns a more detailed description of the executable type.
  virtual std::string GetExeTypeString() const = 0;

  // Returns traits of references given by |type|.
  // Types should be grouped by pools.
  virtual ReferenceTraits GetReferenceTraits(uint8_t type) const = 0;

  // Returns the number of supported ReferenceTraits / ReferenceGroup.
  virtual uint8_t GetReferenceTraitsCount() const = 0;

  // Parses |image_| and initializes internal states. Returns true on success.
  virtual bool Parse() = 0;

  // Creates a generator of ReferenceGroup.
  GroupRange References() { return {this, GetReferenceTraitsCount()}; }
  // Creates a receptor that can receive References of given |type|.
  ReferenceReceptor Receive(uint8_t type) {
    return ReferenceGroup(GetReferenceTraits(type), this).Receive();
  }
  uint8_t GetMaxPoolCount() const;

  // Returns |image_| size. This may shrink after parse.
  offset_t size() const { return offset_t(image_.size()); }

 protected:
  explicit Disassembler(Region image) : image_(image) {}

  // Raw image data. After parse, a disassembler should resize this to contain
  // only the portion containing the executable file it recognizes.
  Region image_;
};

}  // namespace zucchini

#endif  // ZUCCHINI_DISASSEMBLER_H_
