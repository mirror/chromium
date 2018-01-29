// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_VARIANT_H_
#define BASE_VARIANT_H_

#include <algorithm>
#include <utility>

#include "base/logging.h"

// base::variant is a clone of C++17 std::variant. Once C++17 gets available in
// Chromium, base::variant will be replaced with std::variant.
namespace base {

template <size_t>
struct in_place_index_t {};

template <typename>
struct in_place_type_t {};

struct monostate {};

constexpr bool operator<(monostate, monostate) {
  return false;
}

constexpr bool operator>(monostate, monostate) {
  return false;
}

constexpr bool operator<=(monostate, monostate) {
  return true;
}

constexpr bool operator>=(monostate, monostate) {
  return true;
}

constexpr bool operator==(monostate, monostate) {
  return true;
}

constexpr bool operator!=(monostate, monostate) {
  return false;
}

template <typename...>
class variant;

namespace internal {

// TypeMatcher<Ts...> has overloads of Match(T) for each item T in Ts.
template <typename T, typename... Ts>
struct TypeMatcher : TypeMatcher<Ts...> {
  using TypeMatcher<Ts...>::Match;
  static T Match(T);
};

template <typename T>
struct TypeMatcher<T> {
  static T Match(T);
};

template <typename T>
struct ShouldDisableConvertingConstruction : std::false_type {};

template <size_t i>
struct ShouldDisableConvertingConstruction<in_place_index_t<i>>
    : std::true_type {};

template <typename T>
struct ShouldDisableConvertingConstruction<in_place_type_t<T>>
    : std::true_type {};

template <typename... Ts>
struct ShouldDisableConvertingConstruction<variant<Ts...>> : std::true_type {};

struct VariantAccessor;

// Functions below cast their parameters to given type, and call corresponding
// functions on it.
template <typename T>
void DoCopyConstruct(void* obj, const void* other) {
  new (obj) T(*static_cast<const T*>(other));
}

template <typename T>
void DoMoveConstruct(void* obj, void* other) {
  new (obj) T(std::move(*static_cast<T*>(other)));
}

template <typename T>
void DoDestruct(const void* obj) {
  static_cast<const T*>(obj)->~T();
}

template <typename T>
void DoCopyAssign(void* obj, const void* other) {
  *static_cast<T*>(obj) = *static_cast<const T*>(other);
}

template <typename T>
void DoMoveAssign(void* obj, void* other) {
  *static_cast<T*>(obj) = std::move(*static_cast<T*>(other));
}

template <typename T>
void DoSwap(void* left, void* right) {
  using std::swap;
  swap(*static_cast<T*>(left), *static_cast<T*>(right));
}

template <typename T>
bool DoCompareEquals(const void* left, const void* right) {
  return *static_cast<const T*>(left) == *static_cast<const T*>(right);
}

template <typename T>
bool DoCompareLessThan(const void* left, const void* right) {
  return *static_cast<const T*>(left) < *static_cast<const T*>(right);
}

template <size_t i, typename T>
struct VariantHelperBase {
  static constexpr size_t index = i;
  using type = T;
};

// Used by VariantHelperImpl to find a VariantHelperBase for |i|.
template <size_t i, typename T>
VariantHelperBase<i, T> MatchIndex(const VariantHelperBase<i, T>&) {
  return VariantHelperBase<i, T>();
}

// Used by VariantHelperImpl to find a VariantHelperBase for |T|.
template <typename T, size_t i>
VariantHelperBase<i, T> MatchExactType(const VariantHelperBase<i, T>&) {
  return VariantHelperBase<i, T>();
}

template <typename Indices, typename... Ts>
struct VariantHelperImpl;

// VariantHelperImpl inherits all VariantHelperBase<>s of each items in |Ts|, so
// that its reference is convertible any of VariantHelperBase<i, T>.
template <size_t... indices, typename... Ts>
struct VariantHelperImpl<std::index_sequence<indices...>, Ts...>
    : VariantHelperBase<indices, Ts>... {
  static void CallCopyConstructor(size_t i, void* obj, const void* other) {
    using CopyConstructor = void (*)(void*, const void*);
    constexpr CopyConstructor do_copy_ctor[] = {&DoCopyConstruct<Ts>...};
    do_copy_ctor[i](obj, other);
  }

  static void CallMoveConstructor(size_t i, void* obj, void* other) {
    using MoveConstructor = void (*)(void*, void*);
    constexpr MoveConstructor do_move_ctor[] = {&DoMoveConstruct<Ts>...};
    do_move_ctor[i](obj, other);
  }

  static void CallDestructor(size_t i, const void* obj) {
    using Destructor = void (*)(const void*);
    constexpr Destructor do_dtor[] = {&DoDestruct<Ts>...};
    do_dtor[i](obj);
  }

  static void CallCopyAssignment(size_t i, void* obj, const void* other) {
    using CopyAssignment = void (*)(void*, const void*);
    constexpr CopyAssignment do_copy_assign[] = {&DoCopyAssign<Ts>...};
    do_copy_assign[i](obj, other);
  }

  static void CallMoveAssignment(size_t i, void* obj, void* other) {
    using MoveAssignment = void (*)(void*, void*);
    constexpr MoveAssignment do_move_assign[] = {&DoMoveAssign<Ts>...};
    do_move_assign[i](obj, other);
  }

  static void CallSwap(size_t i, void* left, void* right) {
    using Swap = void (*)(void*, void*);
    constexpr Swap do_swap[] = {&DoSwap<Ts>...};
    do_swap[i](left, right);
  }

  static bool CallEquals(size_t i, const void* left, const void* right) {
    using Equals = bool (*)(const void*, const void*);
    constexpr Equals do_equals[] = {&DoCompareEquals<Ts>...};
    return do_equals[i](left, right);
  }

  static bool CallLessThan(size_t i, const void* left, const void* right) {
    using LessThan = bool (*)(const void*, const void*);
    constexpr LessThan do_less_than[] = {&DoCompareLessThan<Ts>...};
    return do_less_than[i](left, right);
  }

  // Returns the index of |T| in |Ts|.
  template <typename T>
  static constexpr size_t IndexForExactType() {
    return decltype(MatchExactType<T>(VariantHelperImpl()))::index;
  }

  // Chooses |T| in |Ts|, and returns the index of it. |T| is chosen by the
  // overload resolution of Foo(U()) where Foo(T) is overloaded for each item
  // in |Ts|.
  template <typename U>
  static constexpr size_t IndexForType() {
    using T = decltype(TypeMatcher<Ts...>::Match(std::declval<U>()));
    return IndexForExactType<T>();
  }

  template <size_t i>
  struct TypeForIndexImpl {
    using HelperBase =
        decltype(MatchIndex<i>(std::declval<VariantHelperImpl>()));
    using type = typename HelperBase::type;
  };

  // Returns |T| in |Ts| which index is |i|.
  template <size_t i>
  using TypeForIndex = typename TypeForIndexImpl<i>::type;
  // template <size_t i>
  // using TypeForIndex =
  //     typename
  //     decltype(MatchIndex<i>(std::declval<VariantHelperImpl>()))::type;
};

template <typename... Ts>
using VariantHelper = VariantHelperImpl<std::index_sequence_for<Ts...>, Ts...>;

}  // namespace internal

// Extracts the |i|th items of the variant type as |type|.
template <size_t i, typename VariantType>
struct variant_alternative;

template <size_t i, typename... Ts>
struct variant_alternative<i, variant<Ts...>> {
  using type =
      typename internal::VariantHelper<Ts...>::template TypeForIndex<i>;
};

template <size_t i, typename... Ts>
struct variant_alternative<i, const variant<Ts...>>
    : variant_alternative<i, variant<Ts...>> {};

template <size_t i, typename T>
using variant_alternative_t = typename variant_alternative<i, T>::type;

// Extracts the number of types in the variant type as |value|.
template <typename VariantType>
struct variant_size;

template <typename... Ts>
struct variant_size<variant<Ts...>>
    : std::integral_constant<size_t, sizeof...(Ts)> {};

template <typename... Ts>
struct variant_size<const variant<Ts...>> : variant_size<variant<Ts...>> {};

// The implementation of base::variant.
template <typename... Ts>
class variant {
 private:
  using Helper = internal::VariantHelper<Ts...>;

  template <size_t i>
  using TypeForIndex = typename Helper::template TypeForIndex<i>;

 public:
  // Default constructor. This constructs the first alternative.
  constexpr variant() : variant(in_place_index_t<0>()) {}

  variant(const variant& other) {
    index_ = other.index_;
    Helper::CallCopyConstructor(index_, data_, other.data_);
  }

  variant(variant&& other) {
    index_ = other.index_;
    Helper::CallMoveConstructor(index_, data_, other.data_);
  }

  // Conversion constructor.
  template <typename T,
            typename =
                std::enable_if_t<!internal::ShouldDisableConvertingConstruction<
                    std::decay_t<T>>::value>>
  constexpr variant(T&& other) {
    constexpr size_t i = Helper::template IndexForType<T&&>();
    using U = TypeForIndex<i>;
    index_ = i;
    new (data_) U(std::forward<T>(other));
  }

  // Construction with explicit type.
  template <typename T, typename... Args>
  constexpr explicit variant(in_place_type_t<T>, Args&&... args) {
    index_ = Helper::template IndexForExactType<T>();
    new (data_) T(std::forward<Args>(args)...);
  }

  template <typename T, typename U, typename... Args>
  constexpr explicit variant(in_place_type_t<T>,
                             std::initializer_list<U> il,
                             Args&&... args) {
    index_ = Helper::template IndexForExactType<T>();
    new (data_) T(il, std::forward<Args>(args)...);
  }

  // Construction with explicit index.
  template <size_t i, typename... Args>
  constexpr explicit variant(in_place_index_t<i>, Args&&... args) {
    index_ = i;
    using T = TypeForIndex<i>;
    new (data_) T(std::forward<Args>(args)...);
  }

  template <size_t i, typename U, typename... Args>
  constexpr explicit variant(in_place_index_t<i>,
                             std::initializer_list<U> il,
                             Args&&... args) {
    index_ = i;
    using T = TypeForIndex<i>;
    new (data_) T(il, std::forward<Args>(args)...);
  }

  ~variant() { Clear(); }

  variant& operator=(const variant& other) {
    if (this == &other)
      return *this;

    if (index_ != other.index_) {
      Clear();
      index_ = other.index_;
      Helper::CallCopyConstructor(index_, data_, other.data_);
    } else {
      Helper::CallCopyAssignment(index_, data_, other.data_);
    }
    return *this;
  }

  variant& operator=(variant&& other) {
    if (this == &other)
      return *this;

    if (index_ != other.index_) {
      Clear();
      index_ = other.index_;
      Helper::CallMoveConstructor(index_, data_, other.data_);
    } else {
      Helper::CallMoveAssignment(index_, data_, other.data_);
    }
    return *this;
  }

  template <typename T,
            typename =
                std::enable_if_t<!internal::ShouldDisableConvertingConstruction<
                    std::decay_t<T>>::value>>
  variant& operator=(T&& other) {
    constexpr size_t i = Helper::template IndexForType<T&&>();
    using U = TypeForIndex<i>;

    if (index_ != i) {
      Clear();
      index_ = i;
      new (data_) U(std::forward<T>(other));
    } else {
      *static_cast<U*>(static_cast<void*>(data_)) = std::forward<T>(other);
    }
    return *this;
  }

  constexpr size_t index() const { return index_; }

  template <size_t i, class... Args>
  TypeForIndex<i>& emplace(Args&&... args) {
    Clear();
    index_ = i;
    using T = TypeForIndex<i>;
    return *new (data_) T(std::forward<Args>(args)...);
  }

  template <size_t i, class U, class... Args>
  TypeForIndex<i>& emplace(std::initializer_list<U> il, Args&&... args) {
    Clear();
    index_ = i;
    using T = TypeForIndex<i>;
    return *new (data_) T(il, std::forward<Args>(args)...);
  }

  template <class T, class... Args>
  T& emplace(Args&&... args) {
    constexpr size_t i = Helper::template IndexForExactType<T>();
    return emplace<i>(std::forward<Args>(args)...);
  }

  template <class T, class U, class... Args>
  T& emplace(std::initializer_list<U> il, Args&&... args) {
    constexpr size_t i = Helper::template IndexForExactType<T>;
    return emplace<i>(il, std::forward<Args>(args)...);
  }

  void swap(variant& other) {
    if (index_ == other.index_) {
      Helper::CallSwap(index_, data_, other.data_);
      return;
    }

    variant tmp(std::move(*this));
    *this = std::move(other);
    other = std::move(tmp);
  }

 private:
  // VariantAccessor allows its whitelisted functions to access |data_|.
  friend struct internal::VariantAccessor;

  void Clear() {
    Helper::CallDestructor(index_, data_);
    index_ = -1;
  }

  alignas(Ts...) uint8_t data_[std::max({sizeof(Ts)...})];
  int index_ = -1;
};

// TODO(tzik): visit();

template <typename... Ts>
void swap(variant<Ts...>& left, variant<Ts...>& right) {
  left.swap(right);
}

// Returns true if |T| is active in |v|.
template <typename T, typename... Ts>
constexpr bool holds_alternative(const variant<Ts...>& v) {
  using Helper = internal::VariantHelper<Ts...>;
  constexpr size_t i = Helper::template IndexForExactType<T>();
  return v.index() == i;
}

// Forward declarations of get<i>(v) for friend declarations in VariantAccessor.
template <size_t i, typename... Ts>
variant_alternative_t<i, variant<Ts...>>& get(variant<Ts...>& v);
template <size_t i, typename... Ts>
const variant_alternative_t<i, variant<Ts...>>& get(const variant<Ts...>& v);
template <size_t i, typename... Ts>
variant_alternative_t<i, variant<Ts...>>&& get(variant<Ts...>&& v);
template <size_t i, typename... Ts>
const variant_alternative_t<i, variant<Ts...>>&& get(const variant<Ts...>&& v);
template <typename... Ts>
bool operator==(const variant<Ts...>& left, const variant<Ts...>& right);
template <typename... Ts>
bool operator<(const variant<Ts...>& left, const variant<Ts...>& right);

namespace internal {

struct VariantAccessor {
 private:
  template <typename Variant>
  static void* data(Variant* v) {
    return v->data_;
  }

  template <typename Variant>
  static const void* data(const Variant* v) {
    return v->data_;
  }

  template <size_t i, typename... Ts>
  friend variant_alternative_t<i, variant<Ts...>>& ::base::get(
      variant<Ts...>& v);
  template <size_t i, typename... Ts>
  friend const variant_alternative_t<i, variant<Ts...>>& ::base::get(
      const variant<Ts...>& v);
  template <size_t i, typename... Ts>
  friend variant_alternative_t<i, variant<Ts...>>&& ::base::get(
      variant<Ts...>&& v);
  template <size_t i, typename... Ts>
  friend const variant_alternative_t<i, variant<Ts...>>&& ::base::get(
      const variant<Ts...>&& v);
  template <typename... Ts>
  friend bool ::base::operator==(const variant<Ts...>& left,
                                 const variant<Ts...>& right);
  template <typename... Ts>
  friend bool ::base::operator<(const variant<Ts...>& left,
                                const variant<Ts...>& right);
};

}  // namespace internal

template <size_t i, typename... Ts>
variant_alternative_t<i, variant<Ts...>>& get(variant<Ts...>& v) {
  DCHECK_EQ(i, v.index());
  using return_type = variant_alternative_t<i, variant<Ts...>>;
  return *static_cast<return_type*>(internal::VariantAccessor::data(&v));
}

template <size_t i, typename... Ts>
const variant_alternative_t<i, variant<Ts...>>& get(const variant<Ts...>& v) {
  DCHECK_EQ(i, v.index());
  using return_type = const variant_alternative_t<i, variant<Ts...>>;
  return *static_cast<return_type*>(internal::VariantAccessor::data(&v));
}

template <size_t i, typename... Ts>
variant_alternative_t<i, variant<Ts...>>&& get(variant<Ts...>&& v) {
  DCHECK_EQ(i, v.index());
  using return_type = variant_alternative_t<i, variant<Ts...>>;
  return std::move(
      *static_cast<return_type*>(internal::VariantAccessor::data(&v)));
}

template <size_t i, typename... Ts>
const variant_alternative_t<i, variant<Ts...>>&& get(const variant<Ts...>&& v) {
  DCHECK_EQ(i, v.index());
  using return_type = const variant_alternative_t<i, variant<Ts...>>;
  return std::move(
      *static_cast<return_type*>(internal::VariantAccessor::data(&v)));
}

template <typename T, typename... Ts>
T& get(variant<Ts...>& v) {
  using Helper = internal::VariantHelper<Ts...>;
  constexpr size_t i = Helper::template IndexForExactType<T>();
  return get<i>(v);
}

template <typename T, typename... Ts>
const T& get(const variant<Ts...>& v) {
  using Helper = internal::VariantHelper<Ts...>;
  constexpr size_t i = Helper::template IndexForExactType<T>();
  return get<i>(v);
}

template <typename T, typename... Ts>
T&& get(variant<Ts...>&& v) {
  using Helper = internal::VariantHelper<Ts...>;
  constexpr size_t i = Helper::template IndexForExactType<T>();
  return get<i>(std::move(v));
}

template <typename T, typename... Ts>
const T&& get(const variant<Ts...>&& v) {
  using Helper = internal::VariantHelper<Ts...>;
  constexpr size_t i = Helper::template IndexForExactType<T>();
  return get<i>(std::move(v));
}

template <size_t i, typename... Ts>
variant_alternative_t<i, variant<Ts...>>* get_if(variant<Ts...>* v) {
  if (!v || v->index() != i)
    return nullptr;
  return &get<i>(*v);
}

template <size_t i, typename... Ts>
variant_alternative_t<i, variant<Ts...>>* get_if(const variant<Ts...>* v) {
  if (!v || v->index() != i)
    return nullptr;
  return &get<i>(*v);
}

template <typename T, typename... Ts>
T* get_if(variant<Ts...>* v) {
  using Helper = internal::VariantHelper<Ts...>;
  constexpr size_t i = Helper::template IndexForType<T>();
  return get_if<i>(v);
}

template <typename T, typename... Ts>
const T* get_if(const variant<Ts...>* v) {
  using Helper = internal::VariantHelper<Ts...>;
  constexpr size_t i = Helper::template IndexForType<T>();
  return get_if<i>(v);
}

template <typename... Ts>
bool operator==(const variant<Ts...>& left, const variant<Ts...>& right) {
  if (left.index() != right.index())
    return false;

  using Helper = internal::VariantHelper<Ts...>;
  return Helper::CallEquals(left.index(),
                            internal::VariantAccessor::data(&left),
                            internal::VariantAccessor::data(&right));
}

template <typename... Ts>
bool operator<(const variant<Ts...>& left, const variant<Ts...>& right) {
  if (left.index() < right.index())
    return true;
  if (left.index() > right.index())
    return false;

  using Helper = internal::VariantHelper<Ts...>;
  return Helper::CallLessThan(left.index(),
                              internal::VariantAccessor::data(&left),
                              internal::VariantAccessor::data(&right));
}

template <typename... Ts>
bool operator!=(const variant<Ts...>& left, const variant<Ts...>& right) {
  return !(left == right);
}

template <typename... Ts>
bool operator<=(const variant<Ts...>& left, const variant<Ts...>& right) {
  return !(left > right);
}

template <typename... Ts>
bool operator>(const variant<Ts...>& left, const variant<Ts...>& right) {
  return right < left;
}

template <typename... Ts>
bool operator>=(const variant<Ts...>& left, const variant<Ts...>& right) {
  return !(left < right);
}

}  // namespace base

#endif  // BASE_VARIANT_H_
