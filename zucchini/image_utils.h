// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZUCCHINI_IMAGE_UTILS_H_
#define ZUCCHINI_IMAGE_UTILS_H_

#include <cassert>
#include <cstdint>
#include <functional>
#include <utility>

#include "zucchini/ranges/meta.h"
#include "zucchini/ranges/view.h"

namespace zucchini {

using RVA = uint32_t;  // will be removed
using rva_t = uint32_t;
using FileOffset = uint32_t;  // will be removed
using offset_t = uint32_t;

constexpr offset_t kNullOffset = offset_t(-1);
constexpr rva_t kNullRva = rva_t(-1);
constexpr uint8_t kNoRefType = 0xff;

using RVAToOffsetTranslator = std::function<offset_t(rva_t)>;
using OffsetToRVATranslator = std::function<rva_t(offset_t)>;

class AddressTranslator {
 public:
  virtual ~AddressTranslator() = default;

  virtual RVAToOffsetTranslator GetRVAToOffsetTranslator() const = 0;
  virtual OffsetToRVATranslator GetOffsetToRVATranslator() const = 0;
};

struct Reference {
  offset_t location;
  offset_t target;
};

inline bool operator==(const Reference& a, const Reference& b) {
  return a.location == b.location && a.target == b.target;
}

// Receptor of References. When called, a receptor shall modify the image file
// according to the Reference given in argument.
using ReferenceReceptor = std::function<void(Reference)>;
// Generator of References.
using ReferenceGenerator = std::function<bool(Reference*)>;

template <class T>
struct TypedReferenceImpl {
  template <class Ref>
  TypedReferenceImpl(uint8_t t, Ref& ref)  // NOLINT for Ref&.
      : type(t), location(ref.location), target(ref.target) {}
  TypedReferenceImpl(uint8_t t, T loc, T tar)
      : type(t), location(loc), target(tar) {}

  template <class TypedRef>
  explicit TypedReferenceImpl(const TypedRef& that)
      : type(that.type), location(that.location), target(that.target) {}

  uint8_t type;
  T location;
  T target;
};

template <class T, class U>
inline bool operator==(const TypedReferenceImpl<T>& a,
                       const TypedReferenceImpl<U>& b) {
  return a.type == b.type && a.location == b.location && a.target == b.target;
}

using MutableTypedReference = TypedReferenceImpl<offset_t&>;
using TypedReference = TypedReferenceImpl<offset_t>;

struct Target {
  template <class T>
  static T Member(T&& t) {
    return t;
  }

  template <class Ref>
  auto operator()(Ref&& ref) const
      -> decltype(Member(std::forward<Ref>(ref).target)) {
    return ref.target;
  }
};

template <class Rng>
auto Targets(Rng&& rng)
    -> decltype(ranges::view::Transform(std::forward<Rng>(rng), Target())) {
  return ranges::view::Transform(std::forward<Rng>(rng), Target());
}

template <class T, int pos, ranges::meta::if_t<(pos >= 0)>* = nullptr>
constexpr T bit() {
  return T(1) << pos;
}

template <class T, int pos, ranges::meta::if_t<(pos < 0)>* = nullptr>
constexpr T bit() {
  return T(1) << (sizeof(T) * 8 - size_t(-pos));
}

// Extracts a single bit at |pos| from integer |v|.
template <int pos, typename T>
T GetBit(T v) {
  return (v >> pos) & 1;
}

// Extracts bits in inclusive range [|lo|, |hi|] from integer |v|, and returns
// the sign-extend result. For example, let the (MSB-first) bits in a 32-bit int
// |v| be:
//   xxxxxxxx xxxxxSii iiiiiiii iyyyyyyy,
//               hi^          lo^       => lo = 7, hi = 18
// To extract "Sii iiiiiiii i", we'd call
//   GetSignedBits<7, 18>(v);
// and obtain the sign-extended result:
//   SSSSSSSS SSSSSSSS SSSSSiii iiiiiiii.
template <int lo, int hi, typename T>
typename std::make_signed<T>::type GetSignedBits(T v) {
  constexpr int kNumBits = sizeof(T) * 8;
  using SIGNED_T = typename std::make_signed<T>::type;
  // Assumes 0 <= |lo| <= |hi| < |kNumBits|.
  // How this works:
  // (1) Shift-left by |kNumBits - 1 - hi| to clear "left" bits.
  // (2) Shift-right by |kNumBits - 1 - hi + lo| to clear "right" bits. The
  //     input is casted to a signed type to perform sign-extension.
  return static_cast<SIGNED_T>(v << (kNumBits - 1 - hi)) >>
         (kNumBits - 1 - hi + lo);
}

// Similar to GetSignedBits(), but returns the zero-extended result. For the
// above example, calling
//   GetUnsignedBits<7, 18>(v);
// results in:
//   00000000 00000000 0000Siii iiiiiiii.
template <int lo, int hi, typename T>
typename std::make_unsigned<T>::type GetUnsignedBits(T v) {
  constexpr int kNumBits = sizeof(T) * 8;
  using UNSIGNED_T = typename std::make_unsigned<T>::type;
  return static_cast<UNSIGNED_T>(v << (kNumBits - 1 - hi)) >>
         (kNumBits - 1 - hi + lo);
}

// Copies bits at |pos| in |v| to all higher bits, and return the result as the
// same int type as |v|.
template <int pos, typename T>
T SignExtend(T v) {
  constexpr int kNumBits = sizeof(T) * 8;
  constexpr int kShift = kNumBits - 1 - pos;
  return static_cast<typename std::make_signed<T>::type>(v << kShift) >> kShift;
}

// Determines whether |v|, if interpreted as a signed integer, is representable
// using |digs| bits. We assume |1 <= digs <= sizeof(T)|.
template <int digs, typename T>
bool SignedFit(T v) {
  return v == SignExtend<digs - 1, T>(v);
}

template <class T>
constexpr T floor(T x, T m) {
  return T(x / m) * m;
}

template <class T>
constexpr T ceil(T x, T m) {
  return T((x + m - 1) / m) * m;
}

// Helper functions to mark an offset_t. We mark with the most significant bit.
inline bool IsMarked(offset_t value) {
  return (value & bit<offset_t, -1>()) != 0;
}
inline offset_t MarkIndex(offset_t value) {
  return value | bit<offset_t, -1>();
}
inline offset_t UnmarkIndex(offset_t value) {
  return value & ~bit<offset_t, -1>();
}

// Returns whether data at |offset| with |length| fit entirely in the range
// |[0, image_size)|.
inline bool CheckDataFit(offset_t offset, size_t length, size_t image_size) {
  assert(length > 0);
  // Must start in |[0, image_size)|.
  if (offset >= image_size)
    return false;
  // Ensure there's enough leftover space. Use subtraction to avoid overflow.
  return image_size - offset >= length;
}

// Returns whether data array at |offset| with |count| entries, each with
// |length| fit entirely in the range |[0, image_size)|.
inline bool CheckDataArrayFit(offset_t offset,
                              size_t count,
                              size_t length,
                              size_t image_size) {
  assert(length > 0);
  // Must start in |[0, image_size)|.
  if (offset >= image_size)
    return false;
  // Ensure there's enough leftover space. Use subtraction and division to avoid
  // overflow.
  return (image_size - offset) / length >= count;
}

}  // namespace zucchini

#endif  // ZUCCHINI_IMAGE_UTILS_H_
