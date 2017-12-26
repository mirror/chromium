// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_PROMISE_TAGGED_UNION_H_
#define BASE_PROMISE_TAGGED_UNION_H_

#include "base/optional.h"

namespace base {

namespace {

// Helper for computing the size needed to store the union.
template <typename... Types>
struct VariantSizeOfHelper;

template <typename First, typename... Rest>
struct VariantSizeOfHelper<First, Rest...> {
  enum {
    kSize = sizeof(First) > VariantSizeOfHelper<Rest...>::kSize
                ? sizeof(First)
                : VariantSizeOfHelper<Rest...>::kSize,
  };
};

template <typename First>
struct VariantSizeOfHelper<First> {
  enum { kSize = sizeof(First) };
};

// Helper for computing the index (if any) of a given type within the parameter
// pack.
template <typename...>
struct VarArgIndexHelper;

template <typename T, typename... Rest>
struct VarArgIndexHelper<T, T, Rest...> {
  enum {
    kIndex = 0,
  };
};

template <typename T>
struct VarArgIndexHelper<T> {
  enum { kIndex = -1 };
};

template <typename T, typename First, typename... Rest>
struct VarArgIndexHelper<T, First, Rest...> {
  enum {
    kIndex = VarArgIndexHelper<T, Rest...>::kIndex != -1
                 ? VarArgIndexHelper<T, Rest...>::kIndex + 1
                 : -1
  };
};

// Helper for extracting the Nth type from a parameter pack.
template <int N, typename... Ts>
using TypeAtHelper = typename std::tuple_element<N, std::tuple<Ts...>>::type;

// Helper for deleting the Nth type from a parameter pack.
template <size_t N, typename...>
struct VariantDestructHelper;

template <size_t N, typename First>
struct VariantDestructHelper<N, First> {
  static void Destruct(size_t idx, void* data) {
    if (idx == N) {
      reinterpret_cast<First*>(data)->~First();
    }
  }
};

template <size_t N, typename First, typename... Rest>
struct VariantDestructHelper<N, First, Rest...> {
  static void Destruct(size_t idx, void* data) {
    if (idx == N) {
      reinterpret_cast<First*>(data)->~First();
    } else {
      VariantDestructHelper<N + 1, Rest...>::Destruct(idx, data);
    }
  }
};

// Helper for moving the Nth type from a parameter pack.
template <size_t N, typename...>
struct VariantMoveHelper;

template <size_t N, typename First>
struct VariantMoveHelper<N, First> {
  static void PlacementMoveConstructor(size_t idx, void* from, void* to) {
    if (idx == N) {
      new (to) First(std::move(*reinterpret_cast<First*>(from)));
    }
  }
};

template <size_t N, typename First, typename... Rest>
struct VariantMoveHelper<N, First, Rest...> {
  static void PlacementMoveConstructor(size_t idx, void* from, void* to) {
    if (idx == N) {
      new (to) First(std::move(*reinterpret_cast<First*>(from)));
    } else {
      VariantMoveHelper<N + 1, Rest...>::PlacementMoveConstructor(idx, from,
                                                                  to);
    }
  }
};

}  // namespace

template <typename... Ts>
class Variant;

template <typename T, typename... TTs>
Optional<T*> GetIf(Variant<TTs...>* variant);

template <typename T, typename... TTs>
Optional<const T*> GetIf(const Variant<TTs...>* variant);

template <typename T, typename... TTs>
T& Get(Variant<TTs...>* variant);

template <typename T, typename... TTs>
const T& Get(const Variant<TTs...>* variant);

struct Monostate {};

template <size_t I>
struct in_place_index_t {
  explicit in_place_index_t() = default;
};

template <typename... Ts>
class Variant {
 public:
  constexpr Variant() : tag_(0) {
    using Type = TypeAtHelper<0, Ts...>;
    new (&union_[0]) Type();
  }

  constexpr Variant(Variant&& other) {
    tag_ = other.tag_;
    VariantMoveHelper<0, Ts...>::PlacementMoveConstructor(
        tag_, &other.union_[0], &union_[0]);
  }

  template <size_t I, class... Args>
  constexpr explicit Variant(in_place_index_t<I>, Args&&... args) : tag_(I) {
    using Type = TypeAtHelper<I, Ts...>;
    new (&union_[0]) Type(std::forward<Args>(args)...);
  }

  template <typename Arg>
  constexpr explicit Variant(Arg&& a) {
    static_assert(VarArgIndexHelper<Arg, Ts...>::kIndex != -1,
                  "Can't construct Variant with a disjoint type.");
    tag_ = VarArgIndexHelper<Arg, Ts...>::kIndex;
    new (&union_[0]) Arg(std::forward<Arg>(a));
  }

  ~Variant() { VariantDestructHelper<0, Ts...>::Destruct(tag_, &union_[0]); }

  template <typename Type, typename... Args>
  constexpr Variant& emplace(Args&&... args) {
    static_assert(VarArgIndexHelper<Type, Ts...>::kIndex != -1,
                  "Can't emplace Variant with a disjoint type.");
    VariantDestructHelper<0, Ts...>::Destruct(tag_, &union_[0]);
    tag_ = VarArgIndexHelper<Type, Ts...>::kIndex;
    new (&union_[0]) Type(std::forward<Args>(args)...);
    return *this;
  }

  template <size_t I, class... Args>
  constexpr auto& emplace(in_place_index_t<I>, Args&&... args) {
    using Type = TypeAtHelper<I, Ts...>;
    VariantDestructHelper<0, Ts...>::Destruct(tag_, &union_[0]);
    tag_ = I;
    return *new (&union_[0]) Type(std::forward<Args>(args)...);
  }

  // returns the zero-based index of the alternative held by the variant
  int index() const { return tag_; }

  template <typename Arg>
  Variant& operator=(Arg&& a) {
    static_assert(VarArgIndexHelper<Arg, Ts...>::kIndex != -1,
                  "Can't assign Variant with a disjoint type.");
    VariantDestructHelper<0, Ts...>::Destruct(tag_, &union_[0]);
    tag_ = VarArgIndexHelper<Arg, Ts...>::kIndex;
    new (&union_[0]) Arg(std::forward<Arg>(a));
    return *this;
  }

 private:
  template <typename T, typename... TTs>
  friend Optional<T*> GetIf(Variant<TTs...>* variant);

  template <typename T, typename... TTs>
  friend Optional<const T*> GetIf(const Variant<TTs...>* variant);

  template <typename T, typename... TTs>
  friend T& Get(Variant<TTs...>* variant);

  template <typename T, typename... TTs>
  friend const T& Get(const Variant<TTs...>* variant);

  int tag_;
  char union_[VariantSizeOfHelper<Ts...>::kSize];
};

template <typename T, typename... Ts>
Optional<T*> GetIf(Variant<Ts...>* variant) {
  if (VarArgIndexHelper<T, Ts...>::kIndex == variant->tag_)
    return reinterpret_cast<T*>(&variant->union_[0]);
  return nullopt;
}

template <typename T, typename... Ts>
Optional<const T*> GetIf(const Variant<Ts...>* variant) {
  if (VarArgIndexHelper<T, Ts...>::kIndex == variant->tag_)
    return reinterpret_cast<const T*>(&variant->union_[0]);
  return nullopt;
}

template <typename T, typename... Ts>
T& Get(Variant<Ts...>* variant) {
  CHECK_EQ((VarArgIndexHelper<T, Ts...>::kIndex), variant->tag_);
  return *reinterpret_cast<T*>(&variant->union_[0]);
}

template <typename T, typename... Ts>
const T& Get(const Variant<Ts...>* variant) {
  CHECK_EQ((VarArgIndexHelper<T, Ts...>::kIndex), variant->tag_);
  return *reinterpret_cast<const T*>(&variant->union_[0]);
}

}  // namespace base

#endif  // BASE_PROMISE_TAGGED_UNION_H_
