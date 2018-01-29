// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_VARIANT_H_
#define BASE_VARIANT_H_

#include <vector>

#include "base/variant_type_ops.h"

namespace base {
namespace internal {
template <typename... Ts>
struct VariantInfoGen;
}  // namespace internal

template <typename... Ts>
class Variant;

template <typename T, typename... TTs>
constexpr std::add_pointer_t<T> GetIf(Variant<TTs...>* variant);

template <typename T, typename... TTs>
constexpr std::add_pointer_t<const T> GetIf(const Variant<TTs...>* variant);

template <size_t I, typename... TTs>
constexpr std::add_pointer_t<internal::TypeAtHelper<I, TTs...>> GetIf(
    Variant<TTs...>* variant);

template <size_t I, typename... TTs>
constexpr std::add_pointer_t<const internal::TypeAtHelper<I, TTs...>> GetIf(
    const Variant<TTs...>* variant);

template <typename T, typename... TTs>
constexpr T& Get(Variant<TTs...>* variant);

template <typename T, typename... TTs>
constexpr const T& Get(const Variant<TTs...>* variant);

template <size_t I, typename... Ts>
constexpr internal::TypeAtHelper<I, Ts...>& Get(Variant<Ts...>* variant);

template <size_t I, typename... Ts>
constexpr const internal::TypeAtHelper<I, Ts...>& Get(
    const Variant<Ts...>* variant);

struct Monostate {};

template <size_t I>
struct in_place_index_t {
  explicit in_place_index_t() = default;
};

class AbstractVariant;

// A subset of std::variant to be replaced with std::variant once C++17 is
// allowed
template <typename... Ts>
class Variant {
 public:
  constexpr Variant() noexcept : index_(0), union_{0} {
    Variant<Ts...>::type_ops_[index_].default_construct(&union_[0]);
  }

  constexpr Variant(Variant& other) noexcept : index_(other.index_), union_{0} {
    Variant<Ts...>::type_ops_[index_].copy_construct(&other.union_[0],
                                                     &union_[0]);
  }

  constexpr Variant(Variant&& other) noexcept
      : index_(other.index_), union_{0} {
    index_ = other.index_;
    Variant<Ts...>::type_ops_[index_].move_construct(&other.union_[0],
                                                     &union_[0]);
  }

  template <typename Arg>
  constexpr Variant(Arg&& a) noexcept {
    using UnderlyingArgType = std::decay_t<Arg>;
    index_ = internal::VarArgIndexHelper<UnderlyingArgType, Ts...>::kIndex;
    new (&union_[0]) UnderlyingArgType(std::forward<Arg>(a));
  }

  template <size_t I, class... Args>
  constexpr explicit Variant(in_place_index_t<I>, Args&&... args) : index_(I) {
    using Type = internal::TypeAtHelper<I, Ts...>;
    new (&union_[0]) Type(std::forward<Args>(args)...);
  }

  ~Variant() { Variant<Ts...>::type_ops_[index_].destruct(&union_[0]); }

  template <typename Type, typename... Args>
  Type& emplace(Args&&... args) {
    static_assert(internal::VarArgIndexHelper<Type, Ts...>::kIndex !=
                      internal::kVarArgIndexHelperInvalidIndex,
                  "Can't emplace Variant with an unrelated type.");
    Variant<Ts...>::type_ops_[index_].destruct(&union_[0]);
    index_ = internal::VarArgIndexHelper<Type, Ts...>::kIndex;
    return *new (&union_[0]) Type(std::forward<Args>(args)...);
  }

  template <size_t I, class... Args>
  internal::TypeAtHelper<I, Ts...>& emplace(in_place_index_t<I>,
                                            Args&&... args) noexcept {
    Variant<Ts...>::type_ops_[index_].destruct(&union_[0]);
    index_ = I;
    return *new (&union_[0])
        internal::TypeAtHelper<I, Ts...>(std::forward<Args>(args)...);
  }

  // Returns the zero-based index of the alternative held by the variant
  constexpr size_t index() const noexcept { return index_; }

  // Strangely this doesn't seem to work if |other| is const, rather the
  // template assignment operator is used which fails to compile.
  Variant& operator=(Variant& other) {
    Variant<Ts...>::type_ops_[index_].destruct(&union_[0]);
    index_ = other.index_;
    Variant<Ts...>::type_ops_[index_].copy_construct(&other.union_[0],
                                                     &union_[0]);
    return *this;
  }

  Variant& operator=(Variant&& other) noexcept {
    Variant<Ts...>::type_ops_[index_].destruct(&union_[0]);
    index_ = other.index_;
    Variant<Ts...>::type_ops_[index_].move_construct(&other.union_[0],
                                                     &union_[0]);
    return *this;
  }

  template <typename Arg>
  Variant& operator=(Arg&& a) noexcept {
    using UnderlyingArgType = std::decay_t<Arg>;
    static_assert(
        internal::VarArgIndexHelper<UnderlyingArgType, Ts...>::kIndex !=
            internal::kVarArgIndexHelperInvalidIndex,
        "Can't assign Variant with a disjoint type.");

    Variant<Ts...>::type_ops_[index_].destruct(&union_[0]);
    index_ = internal::VarArgIndexHelper<UnderlyingArgType, Ts...>::kIndex;
    new (&union_[0]) UnderlyingArgType(std::forward<Arg>(a));
    return *this;
  }

  // Tries to unwrap the type held by |other| and assign it.
  bool TryUnwrapAndAssign(size_t i, AbstractVariant& other);

  static const internal::VariantInfo variant_info_;

 private:
  friend class AbstractVariant;

  template <typename T, typename... TTs>
  friend constexpr std::add_pointer_t<T> GetIf(Variant<TTs...>* variant);

  template <typename T, typename... TTs>
  friend constexpr std::add_pointer_t<const T> GetIf(
      const Variant<TTs...>* variant);

  template <size_t I, typename... TTs>
  friend constexpr std::add_pointer_t<internal::TypeAtHelper<I, TTs...>> GetIf(
      Variant<TTs...>* variant);

  template <size_t I, typename... TTs>
  friend constexpr std::add_pointer_t<const internal::TypeAtHelper<I, TTs...>>
  GetIf(const Variant<TTs...>* variant);

  template <typename T, typename... TTs>
  friend constexpr T& Get(Variant<TTs...>* variant);

  template <typename T, typename... TTs>
  friend constexpr const T& Get(const Variant<TTs...>* variant);

  template <size_t I, typename... TTs>
  friend constexpr internal::TypeAtHelper<I, TTs...>& Get(
      Variant<TTs...>* variant);

  template <size_t I, typename... TTs>
  friend constexpr const internal::TypeAtHelper<I, TTs...>& Get(
      const Variant<TTs...>* variant);

  template <typename... TTs>
  friend struct internal::VariantInfoGen;

  static constexpr internal::VariantTypeOps type_ops_[sizeof...(Ts)] = {
      internal::VariantTypeOps::Create<Ts>()...};

  size_t index_;
  char union_[internal::VariantSizeOfHelper<Ts...>::kSize];
};

template <typename... Ts>
constexpr internal::VariantTypeOps Variant<Ts...>::type_ops_[sizeof...(Ts)];

namespace internal {
template <typename... Ts>
struct VariantInfoGen {
  static constexpr internal::VariantInfo variant_info_ = {
      Variant<Ts...>::type_ops_, sizeof...(Ts),
      offsetof(base::Variant<Ts...>, index_),
      offsetof(base::Variant<Ts...>, union_)};
};

template <typename... Ts>
constexpr internal::VariantInfo VariantInfoGen<Ts...>::variant_info_;

template <typename... Ts>
struct IsVariant<base::Variant<Ts...>> {
  static constexpr bool value = true;
  static constexpr const VariantInfo* variant_info =
      &VariantInfoGen<Ts...>::variant_info_;
};
}  // namespace internal

template <typename T>
T* GetIf(AbstractVariant* abstract_variant);

template <typename T>
T& Get(AbstractVariant* abstract_variant);

template <typename T>
const T* GetIf(const AbstractVariant* abstract_variant);

template <typename T>
const T& Get(const AbstractVariant* abstract_variant);

// Similar to Variant but an AbstractVariant can hold any type.
class BASE_EXPORT AbstractVariant {
 public:
  template <typename... Ts>
  struct ConstructWith {};

  template <typename... Ts>
  AbstractVariant(ConstructWith<Ts...>)
      : type_ops_(Variant<Ts...>::type_ops_),
        num_types_(sizeof...(Ts)),
        index_(0),
        union_(internal::VariantSizeOfHelper<Ts...>::kSize) {
    type_ops_[index_].default_construct(&union_[0]);
  }

  AbstractVariant(AbstractVariant&& other);
  AbstractVariant(const AbstractVariant& other) = delete;

  ~AbstractVariant();

  AbstractVariant& operator=(const AbstractVariant& other) = delete;

  size_t index() const { return index_; }

  uint32_t type_id() const { return type_ops_[index_].type_id; }

  template <typename T>
  bool TryAssign(T&& other) {
    uint32_t other_type_id = internal::TypeId<std::decay_t<T>>::Id();
    for (size_t i = 0; i < num_types_; i++) {
      if (type_ops_[i].type_id == other_type_id) {
        type_ops_[index_].destruct(&union_[0]);
        new (&union_[0]) std::decay_t<T>(std::forward<T>(other));
        index_ = i;
        return true;
      }
    }
    return false;
  }

  template <typename T, typename... Args>
  bool TryEmplace(Args&&... args) {
    uint32_t variant_type_id = internal::TypeId<T>::Id();
    for (size_t i = 0; i < num_types_; i++) {
      if (type_ops_[i].type_id == variant_type_id) {
        type_ops_[index_].destruct(&union_[0]);
        new (&union_[0]) std::decay_t<T>(std::forward<Args>(args)...);
        index_ = i;
        return true;
      }
    }
    return false;
  }

  bool TryAssign(AbstractVariant& other) {
    uint32_t other_type_id = other.type_id();
    for (size_t i = 0; i < num_types_; i++) {
      if (type_ops_[i].type_id == other_type_id) {
        type_ops_[index_].destruct(&union_[0]);
        index_ = i;
        if (type_ops_[i].is_move_constructable) {
          type_ops_[i].move_construct(&other.union_[0], &union_[0]);
        } else {
          DCHECK(type_ops_[i].is_copy_constructable);
          type_ops_[i].copy_construct(&other.union_[0], &union_[0]);
        }
        return true;
      }
    }
    return false;
  }

  template <typename... Ts>
  bool TryAssign(Variant<Ts...>& other) {
    const internal::VariantTypeOps& other_type_ops =
        Variant<Ts...>::type_ops_[other.index_];
    for (size_t i = 0; i < num_types_; i++) {
      if (type_ops_[i].type_id == other_type_ops.type_id) {
        type_ops_[index_].destruct(&union_[0]);
        index_ = i;
        if (other_type_ops.is_move_constructable) {
          other_type_ops.move_construct(&other.union_[0], &union_[0]);
        } else {
          DCHECK(other_type_ops.is_copy_constructable);
          other_type_ops.copy_construct(&other.union_[0], &union_[0]);
        }
        return true;
      }
    }
    return false;
  }

  // Tries to assign |other| and if that fails tries to emplace type |i| iff
  // that is a Variant.
  bool TryAssignOrUnwrapAndEmplace(AbstractVariant& other, size_t i) {
    if (TryAssign(other))
      return true;

    if (i >= num_types_)
      return false;

    const internal::VariantInfo* variant_info = type_ops_[i].variant_info;
    if (!variant_info)
      return false;

    const internal::VariantTypeOps* other_variant_info =
        other.type_ops_[other.index_].alias_info;
    if (!other_variant_info)
      return false;

    uint32_t other_type_id = other_variant_info->type_id;
    for (size_t j = 0; j < variant_info->num_variants; j++) {
      const internal::VariantTypeOps* variant_type_ops =
          &variant_info->type_ops[j];
      if (variant_type_ops->type_id == other_type_id) {
        if (i != index_) {
          type_ops_[index_].destruct(&union_[0]);
          index_ = i;
          type_ops_[index_].default_construct(&union_[0]);
        }
        size_t* variant_index = reinterpret_cast<size_t*>(
            &union_[variant_info->index_offset_bytes]);
        void* variant_union = &union_[variant_info->union_offset_bytes];
        variant_info->type_ops[*variant_index].destruct(variant_union);
        *variant_index = j;
        if (other_variant_info->is_move_constructable) {
          other_variant_info->move_construct(&other.union_[0], variant_union);
        } else {
          DCHECK(other_variant_info->is_copy_constructable);
          other_variant_info->copy_construct(&other.union_[0], variant_union);
        }
        return true;
      }
    }

    return false;
  }

 private:
  template <typename... Ts>
  friend class Variant;

  template <typename T>
  friend T* GetIf(AbstractVariant* abstract_variant);

  template <typename T>
  friend T& Get(AbstractVariant* abstract_variant);

  template <typename T>
  friend const T* GetIf(const AbstractVariant* abstract_variant);

  template <typename T>
  friend const T& Get(const AbstractVariant* abstract_variant);

  const internal::VariantTypeOps* type_ops_;
  size_t num_types_;
  size_t index_;
  std::vector<char> union_;
};

template <typename... Ts>
bool Variant<Ts...>::TryUnwrapAndAssign(size_t i, AbstractVariant& other) {
  if (i >= sizeof...(Ts))
    return false;

  const internal::VariantTypeOps* other_variant_info =
      other.type_ops_[other.index_].alias_info;
  if (!other_variant_info)
    return false;

  uint32_t other_type_id = other_variant_info->type_id;
  for (size_t j = 0; j < sizeof...(Ts); j++) {
    if (Variant<Ts...>::type_ops_[j].type_id == other_type_id) {
      Variant<Ts...>::type_ops_[index_].destruct(&union_[0]);
      index_ = j;
      if (Variant<Ts...>::type_ops_[j].is_move_constructable) {
        Variant<Ts...>::type_ops_[j].move_construct(&other.union_[0],
                                                    &union_[0]);
      } else {
        DCHECK(Variant<Ts...>::type_ops_[j].is_copy_constructable);
        Variant<Ts...>::type_ops_[j].copy_construct(&other.union_[0],
                                                    &union_[0]);
      }
      return true;
    }
  }

  return false;
}

template <typename T>
T* GetIf(AbstractVariant* abstract_variant) {
  if (abstract_variant->type_id() != internal::TypeId<T>::Id())
    return nullptr;
  return *reinterpret_cast<T*>(&abstract_variant->union_[0]);
}

template <typename T>
T& Get(AbstractVariant* abstract_variant) {
  DCHECK_EQ(abstract_variant->type_id(), internal::TypeId<T>::Id());
  return *reinterpret_cast<T*>(&abstract_variant->union_[0]);
}

template <typename T>
const T* GetIf(const AbstractVariant* abstract_variant) {
  if (abstract_variant->type_id() != internal::TypeId<T>::Id())
    return nullptr;
  return *reinterpret_cast<const T*>(&abstract_variant->union_[0]);
}

template <typename T>
const T& Get(const AbstractVariant* abstract_variant) {
  DCHECK_EQ(abstract_variant->type_id(), internal::TypeId<T>::Id());
  return *reinterpret_cast<const T*>(&abstract_variant->union_[0]);
}

template <typename T, typename... Ts>
constexpr std::add_pointer_t<T> GetIf(Variant<Ts...>* variant) {
  static_assert(internal::VarArgIndexHelper<T, Ts...>::kIndex !=
                    internal::kVarArgIndexHelperInvalidIndex,
                "Can't Get an unrelated type from a Variant.");
  if (internal::VarArgIndexHelper<T, Ts...>::kIndex == variant->index_)
    return reinterpret_cast<T*>(&variant->union_[0]);
  return nullptr;
}

template <typename T, typename... Ts>
constexpr std::add_pointer_t<const T> GetIf(const Variant<Ts...>* variant) {
  static_assert(internal::VarArgIndexHelper<T, Ts...>::kIndex !=
                    internal::kVarArgIndexHelperInvalidIndex,
                "Can't Get an unrelated type from a Variant.");
  if (internal::VarArgIndexHelper<T, Ts...>::kIndex == variant->index_)
    return reinterpret_cast<const T*>(&variant->union_[0]);
  return nullptr;
}

template <size_t I, typename... TTs>
constexpr std::add_pointer_t<internal::TypeAtHelper<I, TTs...>> GetIf(
    Variant<TTs...>* variant) {
  if (I == variant->index_) {
    return reinterpret_cast<
        std::add_pointer_t<internal::TypeAtHelper<I, TTs...>>>(
        &variant->union_[0]);
  }
  return nullptr;
}

template <size_t I, typename... TTs>
constexpr std::add_pointer_t<const internal::TypeAtHelper<I, TTs...>> GetIf(
    const Variant<TTs...>* variant) {
  if (I == variant->index_) {
    return reinterpret_cast<
        const std::add_pointer_t<internal::TypeAtHelper<I, TTs...>>>(
        &variant->union_[0]);
  }
  return nullptr;
}

template <size_t I, typename... Ts>
constexpr internal::TypeAtHelper<I, Ts...>& Get(Variant<Ts...>* variant) {
  DCHECK_EQ(I, variant->index_);
  return *reinterpret_cast<internal::TypeAtHelper<I, Ts...>*>(
      &variant->union_[0]);
}

template <size_t I, typename... Ts>
constexpr const internal::TypeAtHelper<I, Ts...>& Get(
    const Variant<Ts...>* variant) {
  DCHECK_EQ(I, variant->index_);
  return *reinterpret_cast<const internal::TypeAtHelper<I, Ts...>*>(
      &variant->union_[0]);
}

template <typename T, typename... Ts>
constexpr T& Get(Variant<Ts...>* variant) {
  static_assert(internal::VarArgIndexHelper<T, Ts...>::kIndex !=
                    internal::kVarArgIndexHelperInvalidIndex,
                "Can't Get an unrelated type from a Variant.");

  DCHECK_EQ((internal::VarArgIndexHelper<T, Ts...>::kIndex),
            static_cast<int>(variant->index_));
  return *reinterpret_cast<T*>(&variant->union_[0]);
}

template <typename T, typename... Ts>
constexpr const T& Get(const Variant<Ts...>* variant) {
  static_assert(internal::VarArgIndexHelper<T, Ts...>::kIndex !=
                    internal::kVarArgIndexHelperInvalidIndex,
                "Can't Get an unrelated type from a Variant.");

  DCHECK_EQ((internal::VarArgIndexHelper<T, Ts...>::kIndex),
            static_cast<int>(variant->index_));
  return *reinterpret_cast<const T*>(&variant->union_[0]);
}

}  // namespace base

#endif  // BASE_VARIANT_H_
