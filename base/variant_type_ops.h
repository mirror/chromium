// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_VARIANT_TYPE_OPS_H_
#define BASE_VARIANT_TYPE_OPS_H_

#include <tuple>

#include "base/logging.h"

namespace base {

template <typename... Ts>
class Variant;

namespace internal {

template <typename T>
class TypeId {
 public:
  static constexpr uint32_t Id() {
#ifdef COMPILER_GCC
    return Djb2Hash(__PRETTY_FUNCTION__);
#else
    return Djb2Hash(__FUNCSIG__);
#endif
  }

 private:
  static constexpr uint32_t Djb2Hash(const char* s) {
    uint32_t hash = 5381;
    for (uint32_t i = 0; s[i]; i++)
      hash = static_cast<uint32_t>(static_cast<uint64_t>(hash) * 33) + s[i];
    return hash;
  }
};

struct VariantTypeOps;

struct VariantInfo {
  const VariantTypeOps* type_ops;
  size_t num_variants;
  size_t index_offset_bytes;
  size_t union_offset_bytes;
};

// Helper for classifying if a type is a Variant or not.
template <typename T>
struct IsVariant {
  static constexpr bool value = false;
  static constexpr const VariantInfo* variant_info = nullptr;
};

// Helper for computing the size needed to store the union.
template <typename... Types>
struct VariantSizeOfHelper;

template <>
struct VariantSizeOfHelper<> {
  enum { kSize = 0 };
};

template <typename First, typename... Rest>
struct VariantSizeOfHelper<First, Rest...> {
  enum {
    kSize = sizeof(First) > VariantSizeOfHelper<Rest...>::kSize
                ? sizeof(First)
                : VariantSizeOfHelper<Rest...>::kSize,
  };
};

// Helper for computing the index (if any) of a given type within the parameter
// pack.
template <typename...>
struct VarArgIndexHelper;

enum { kVarArgIndexHelperInvalidIndex = 0xffffffff };

template <typename T, typename... Rest>
struct VarArgIndexHelper<T, T, Rest...> {
  enum {
    kIndex = 0,
  };
};

template <typename T>
struct VarArgIndexHelper<T> {
  enum { kIndex = kVarArgIndexHelperInvalidIndex };
};

template <typename T, typename First, typename... Rest>
struct VarArgIndexHelper<T, First, Rest...> {
  enum {
    kIndex =
        VarArgIndexHelper<T, Rest...>::kIndex != kVarArgIndexHelperInvalidIndex
            ? VarArgIndexHelper<T, Rest...>::kIndex + 1
            : kVarArgIndexHelperInvalidIndex
  };
};

// Helper for extracting the Nth type from a parameter pack.
template <size_t N, typename... Ts>
using TypeAtHelper = typename std::tuple_element<N, std::tuple<Ts...>>::type;

// Helper implementing type operations needed by base::Variant and
// base::AbstractVariant.
template <typename T>
class VariantTypeOpsImpl {
 public:
  static void DefaultConstruct(void* addr) { new (addr) T(); }

  template <typename TT = T>
  static std::enable_if_t<std::is_copy_constructible<TT>::value, void>
  CopyConstruct(const void* from, void* to) {
    new (to) T(*reinterpret_cast<const T*>(from));
  }

  template <typename TT = T>
  static std::enable_if_t<!std::is_copy_constructible<TT>::value, void>
  CopyConstruct(const void* from, void* to) {
    DCHECK(false) << "Type is not copy constructible.";
  }

  template <typename TT = T>
  static std::enable_if_t<std::is_move_constructible<TT>::value, void>
  MoveConstruct(void* from, void* to) {
    new (to) T(std::move(*reinterpret_cast<T*>(from)));
  }

  template <typename TT = T>
  static std::enable_if_t<!std::is_move_constructible<TT>::value, void>
  MoveConstruct(void* from, void* to) {
    DCHECK(false) << "Type is not move constructible.";
  }

  static void Destruct(void* addr) { static_cast<T*>(addr)->~T(); }

#ifndef NDEBUG
  static const char* TypeName() {
#ifdef COMPILER_GCC
    return __PRETTY_FUNCTION__;
#else
    return __FUNCSIG__;
#endif
  }
#endif
};

template <typename T>
struct UnwrapAliasType {
  constexpr static const VariantTypeOps* alias_info = nullptr;
};

// Compile time initialized structure containing type operations for a
// particular type. Ideally this would be a pure virtual class but those can't
// be compile time initialized.
struct VariantTypeOps {
  using DefaultConstructFnPtr = void (*)(void* addr);
  using CopyConstructFnPtr = void (*)(const void* from, void* to);
  using MoveConstructFnPtr = void (*)(void* from, void* to);
  using DestructFnPtr = void (*)(void* addr);
  using TypeNameFnPtr = const char* (*)();

  const DefaultConstructFnPtr default_construct;
  const CopyConstructFnPtr copy_construct;
  const MoveConstructFnPtr move_construct;
  const DestructFnPtr destruct;
#ifndef NDEBUG
  const TypeNameFnPtr get_name;
#endif

  const uint32_t type_id;
  const bool is_copy_constructable;
  const bool is_move_constructable;

  const VariantTypeOps* const alias_info;

  // This will be null unless T is a variant.
  const VariantInfo* const variant_info;

  template <typename T>
  constexpr static VariantTypeOps Create() {
    return VariantTypeOps{&VariantTypeOpsImpl<T>::DefaultConstruct,
                          &VariantTypeOpsImpl<T>::CopyConstruct,
                          &VariantTypeOpsImpl<T>::MoveConstruct,
                          &VariantTypeOpsImpl<T>::Destruct,
#ifndef NDEBUG
                          &VariantTypeOpsImpl<T>::TypeName,
#endif
                          TypeId<T>::Id(),
                          std::is_copy_constructible<T>::value,
                          std::is_move_constructible<T>::value,
                          UnwrapAliasType<T>::alias_info,
                          IsVariant<T>::variant_info};
  }
};

}  // namespace internal
}  // namespace base

#endif  // BASE_VARIANT_TYPE_OPS_H_
