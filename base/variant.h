// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_VARIANT_H_
#define BASE_VARIANT_H_

#include <tuple>

#include "base/logging.h"

namespace base {

namespace {

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

// Helper for performing operations on the Nth type from a parameter pack.
template <size_t N, typename...>
struct VariantHelper;

template <size_t N>
struct VariantHelper<N> {
  static constexpr void Destruct(size_t idx, void* data) noexcept {}

  static constexpr void Move(size_t idx, void* from, void* to) noexcept {}

  static constexpr void Copy(size_t idx, const void* from, void* to) noexcept {}
};

template <size_t N, typename First, typename... Rest>
struct VariantHelper<N, First, Rest...> {
  static constexpr void Destruct(size_t idx, void* data) noexcept {
    if (idx == N) {
      reinterpret_cast<First*>(data)->~First();
    } else {
      VariantHelper<N + 1, Rest...>::Destruct(idx, data);
    }
  }

  static constexpr void Move(size_t idx, void* from, void* to) noexcept {
    if (idx == N) {
      new (to) First(std::move(*reinterpret_cast<First*>(from)));
    } else {
      VariantHelper<N + 1, Rest...>::Move(idx, from, to);
    }
  }

  static constexpr void Copy(size_t idx, const void* from, void* to) noexcept {
    if (idx == N) {
      new (to) First(*reinterpret_cast<const First*>(from));
    } else {
      VariantHelper<N + 1, Rest...>::Copy(idx, from, to);
    }
  }
};

}  // namespace

template <typename... Ts>
class Variant;

template <typename T, typename... TTs>
constexpr std::add_pointer_t<T> GetIf(Variant<TTs...>* variant);

template <typename T, typename... TTs>
constexpr std::add_pointer_t<const T> GetIf(const Variant<TTs...>* variant);

template <size_t I, typename... TTs>
constexpr std::add_pointer_t<TypeAtHelper<I, TTs...>> GetIf(
    Variant<TTs...>* variant);

template <size_t I, typename... TTs>
constexpr std::add_pointer_t<const TypeAtHelper<I, TTs...>> GetIf(
    const Variant<TTs...>* variant);

template <typename T, typename... TTs>
constexpr T& Get(Variant<TTs...>* variant);

template <typename T, typename... TTs>
constexpr const T& Get(const Variant<TTs...>* variant);

template <size_t I, typename... Ts>
constexpr TypeAtHelper<I, Ts...>& Get(Variant<Ts...>* variant);

template <size_t I, typename... Ts>
constexpr const TypeAtHelper<I, Ts...>& Get(const Variant<Ts...>* variant);

struct Monostate {};

template <size_t I>
struct in_place_index_t {
  explicit in_place_index_t() = default;
};

// A subset of std::variant to be replaced with std::variant once C++17 is
// allowed
template <typename... Ts>
class Variant {
 public:
  constexpr Variant() noexcept : index_(0) {
    using Type = TypeAtHelper<0, Ts...>;
    new (&union_[0]) Type();
  }

  constexpr Variant(Variant& other) noexcept {
    index_ = other.index_;
    VariantHelper<0, Ts...>::Copy(index_, &other.union_[0], &union_[0]);
  }

  constexpr Variant(Variant&& other) noexcept {
    index_ = other.index_;
    VariantHelper<0, Ts...>::Move(index_, &other.union_[0], &union_[0]);
  }

  template <typename Arg>
  constexpr Variant(Arg&& a) noexcept {
    using UnderlyingArgType = std::decay_t<Arg>;
    static_assert(VarArgIndexHelper<UnderlyingArgType, Ts...>::kIndex !=
                      kVarArgIndexHelperInvalidIndex,
                  "Can't construct Variant with an unrelated type.");
    index_ = VarArgIndexHelper<UnderlyingArgType, Ts...>::kIndex;
    new (&union_[0]) UnderlyingArgType(std::forward<Arg>(a));
  }

  template <size_t I, class... Args>
  constexpr explicit Variant(in_place_index_t<I>, Args&&... args) : index_(I) {
    using Type = TypeAtHelper<I, Ts...>;
    new (&union_[0]) Type(std::forward<Args>(args)...);
  }

  ~Variant() { VariantHelper<0, Ts...>::Destruct(index_, &union_[0]); }

  template <typename Type, typename... Args>
  constexpr Type& emplace(Args&&... args) {
    static_assert(VarArgIndexHelper<Type, Ts...>::kIndex !=
                      kVarArgIndexHelperInvalidIndex,
                  "Can't emplace Variant with an unrelated type.");
    VariantHelper<0, Ts...>::Destruct(index_, &union_[0]);
    index_ = VarArgIndexHelper<Type, Ts...>::kIndex;
    return *new (&union_[0]) Type(std::forward<Args>(args)...);
  }

  template <size_t I, class... Args>
  constexpr TypeAtHelper<I, Ts...>& emplace(in_place_index_t<I>,
                                            Args&&... args) noexcept {
    VariantHelper<0, Ts...>::Destruct(index_, &union_[0]);
    index_ = I;
    return *new (&union_[0])
        TypeAtHelper<I, Ts...>(std::forward<Args>(args)...);
  }

  // Returns the zero-based index of the alternative held by the variant
  constexpr size_t index() const noexcept { return index_; }

  // Strangely this doesn't seem to work if |other| is const, rather the
  // template assignment operator is used which fails to compile.
  Variant& operator=(Variant& other) {
    VariantHelper<0, Ts...>::Destruct(index_, &union_[0]);
    index_ = other.index_;
    VariantHelper<0, Ts...>::Copy(index_, &other.union_[0], &union_[0]);
    return *this;
  }

  Variant& operator=(Variant&& other) noexcept {
    VariantHelper<0, Ts...>::Destruct(index_, &union_[0]);
    index_ = other.index_;
    VariantHelper<0, Ts...>::Move(index_, &other.union_[0], &union_[0]);
    return *this;
  }

  template <typename Arg>
  Variant& operator=(Arg&& a) noexcept {
    using UnderlyingArgType = std::decay_t<Arg>;
    static_assert(VarArgIndexHelper<UnderlyingArgType, Ts...>::kIndex !=
                      kVarArgIndexHelperInvalidIndex,
                  "Can't assign Variant with a disjoint type.");

    VariantHelper<0, Ts...>::Destruct(index_, &union_[0]);
    index_ = VarArgIndexHelper<UnderlyingArgType, Ts...>::kIndex;
    new (&union_[0]) UnderlyingArgType(std::forward<Arg>(a));
    return *this;
  }

 private:
  template <typename T, typename... TTs>
  friend constexpr std::add_pointer_t<T> GetIf(Variant<TTs...>* variant);

  template <typename T, typename... TTs>
  friend constexpr std::add_pointer_t<const T> GetIf(
      const Variant<TTs...>* variant);

  template <size_t I, typename... TTs>
  friend constexpr std::add_pointer_t<TypeAtHelper<I, TTs...>> GetIf(
      Variant<TTs...>* variant);

  template <size_t I, typename... TTs>
  friend constexpr std::add_pointer_t<const TypeAtHelper<I, TTs...>> GetIf(
      const Variant<TTs...>* variant);

  template <typename T, typename... TTs>
  friend constexpr T& Get(Variant<TTs...>* variant);

  template <typename T, typename... TTs>
  friend constexpr const T& Get(const Variant<TTs...>* variant);

  template <size_t I, typename... TTs>
  friend constexpr TypeAtHelper<I, TTs...>& Get(Variant<TTs...>* variant);

  template <size_t I, typename... TTs>
  friend constexpr const TypeAtHelper<I, TTs...>& Get(
      const Variant<TTs...>* variant);

  size_t index_;
  char union_[VariantSizeOfHelper<Ts...>::kSize];
};

template <typename T, typename... Ts>
constexpr std::add_pointer_t<T> GetIf(Variant<Ts...>* variant) {
  static_assert(
      VarArgIndexHelper<T, Ts...>::kIndex != kVarArgIndexHelperInvalidIndex,
      "Can't Get an unrelated type from a Variant.");
  if (VarArgIndexHelper<T, Ts...>::kIndex == variant->index_)
    return reinterpret_cast<T*>(&variant->union_[0]);
  return nullptr;
}

template <typename T, typename... Ts>
constexpr std::add_pointer_t<const T> GetIf(const Variant<Ts...>* variant) {
  static_assert(
      VarArgIndexHelper<T, Ts...>::kIndex != kVarArgIndexHelperInvalidIndex,
      "Can't Get an unrelated type from a Variant.");
  if (VarArgIndexHelper<T, Ts...>::kIndex == variant->index_)
    return reinterpret_cast<const T*>(&variant->union_[0]);
  return nullptr;
}

template <size_t I, typename... TTs>
constexpr std::add_pointer_t<TypeAtHelper<I, TTs...>> GetIf(
    Variant<TTs...>* variant) {
  if (I == variant->index_) {
    return reinterpret_cast<std::add_pointer_t<TypeAtHelper<I, TTs...>>>(
        &variant->union_[0]);
  }
  return nullptr;
}

template <size_t I, typename... TTs>
constexpr std::add_pointer_t<const TypeAtHelper<I, TTs...>> GetIf(
    const Variant<TTs...>* variant) {
  if (I == variant->index_) {
    return reinterpret_cast<const std::add_pointer_t<TypeAtHelper<I, TTs...>>>(
        &variant->union_[0]);
  }
  return nullptr;
}

template <size_t I, typename... Ts>
constexpr TypeAtHelper<I, Ts...>& Get(Variant<Ts...>* variant) {
  DCHECK_EQ(I, variant->index_);
  return *reinterpret_cast<TypeAtHelper<I, Ts...>*>(&variant->union_[0]);
}

template <size_t I, typename... Ts>
constexpr const TypeAtHelper<I, Ts...>& Get(const Variant<Ts...>* variant) {
  DCHECK_EQ(I, variant->index_);
  return *reinterpret_cast<const TypeAtHelper<I, Ts...>*>(&variant->union_[0]);
}

template <typename T, typename... Ts>
constexpr T& Get(Variant<Ts...>* variant) {
  static_assert(
      VarArgIndexHelper<T, Ts...>::kIndex != kVarArgIndexHelperInvalidIndex,
      "Can't Get an unrelated type from a Variant.");

  DCHECK_EQ((VarArgIndexHelper<T, Ts...>::kIndex), variant->index_);
  return *reinterpret_cast<T*>(&variant->union_[0]);
}

template <typename T, typename... Ts>
constexpr const T& Get(const Variant<Ts...>* variant) {
  static_assert(
      VarArgIndexHelper<T, Ts...>::kIndex != kVarArgIndexHelperInvalidIndex,
      "Can't Get an unrelated type from a Variant.");

  DCHECK_EQ((VarArgIndexHelper<T, Ts...>::kIndex), variant->index_);
  return *reinterpret_cast<const T*>(&variant->union_[0]);
}

}  // namespace base

#endif  // BASE_VARIANT_H_
