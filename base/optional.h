// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_OPTIONAL_H_
#define BASE_OPTIONAL_H_

#include <type_traits>

#include "base/logging.h"

namespace base {

// Specification:
// http://en.cppreference.com/w/cpp/utility/optional/in_place_t
struct in_place_t {};

// Specification:
// http://en.cppreference.com/w/cpp/utility/optional/nullopt_t
struct nullopt_t {
  constexpr explicit nullopt_t(int) {}
};

// Specification:
// http://en.cppreference.com/w/cpp/utility/optional/in_place
constexpr in_place_t in_place = {};

// Specification:
// http://en.cppreference.com/w/cpp/utility/optional/nullopt
constexpr nullopt_t nullopt(0);

// Forward declaration to implement IsOptionalConvertible.
template <typename T>
class Optional;

namespace internal {

template <typename T, bool = std::is_trivially_destructible<T>::value>
struct OptionalStorage {
  // Initializing |empty_| here instead of using default member initializing
  // to avoid errors in g++ 4.8.
  constexpr OptionalStorage() : empty_('\0') {}

  constexpr explicit OptionalStorage(const T& value)
      : is_null_(false), value_(value) {}

  constexpr explicit OptionalStorage(T&& value)
      : is_null_(false), value_(std::move(value)) {}

  template <class... Args>
  constexpr explicit OptionalStorage(base::in_place_t, Args&&... args)
      : is_null_(false), value_(std::forward<Args>(args)...) {}

  // When T is not trivially destructible we must call its
  // destructor before deallocating its memory.
  ~OptionalStorage() {
    if (!is_null_)
      value_.~T();
  }

  bool is_null_ = true;
  union {
    // |empty_| exists so that the union will always be initialized, even when
    // it doesn't contain a value. Union members must be initialized for the
    // constructor to be 'constexpr'.
    char empty_;
    T value_;
  };
};

template <typename T>
struct OptionalStorage<T, true> {
  // Initializing |empty_| here instead of using default member initializing
  // to avoid errors in g++ 4.8.
  constexpr OptionalStorage() : empty_('\0') {}

  constexpr explicit OptionalStorage(const T& value)
      : is_null_(false), value_(value) {}

  constexpr explicit OptionalStorage(T&& value)
      : is_null_(false), value_(std::move(value)) {}

  template <class... Args>
  constexpr explicit OptionalStorage(base::in_place_t, Args&&... args)
      : is_null_(false), value_(std::forward<Args>(args)...) {}

  // When T is trivially destructible (i.e. its destructor does nothing) there
  // is no need to call it. Explicitly defaulting the destructor means it's not
  // user-provided. Those two together make this destructor trivial.
  ~OptionalStorage() = default;

  bool is_null_ = true;
  union {
    // |empty_| exists so that the union will always be initialized, even when
    // it doesn't contain a value. Union members must be initialized for the
    // constructor to be 'constexpr'.
    char empty_;
    T value_;
  };
};

template <typename T, typename U>
struct IsOptionalConvertible {
  constexpr static bool value =
      std::is_constructible<T, Optional<U>&>::value ||
      std::is_constructible<T, const Optional<U>&>::value ||
      std::is_constructible<T, Optional<U>&&>::value ||
      std::is_constructible<T, const Optional<U>&&>::value ||
      std::is_convertible<Optional<U>&, T>::value ||
      std::is_convertible<const Optional<U>&, T>::value ||
      std::is_convertible<Optional<U>&&, T>::value ||
      std::is_convertible<const Optional<U>&&, T>::value;
};

template <typename T, typename U>
struct IsOptionalAssignable {
  constexpr static bool value =
      IsOptionalConvertible<T, U>::value ||
      std::is_assignable<T&, Optional<U>&>::value ||
      std::is_assignable<T&, const Optional<U>&>::value ||
      std::is_assignable<T&, Optional<U>&&>::value ||
      std::is_assignable<T&, const Optional<U>&&>::value;
};

// Base class to implement "delete" copy/move constructor/assignment-operator
// conditionally.
// TODO comment.
template <typename T>
class OptionalBase {
 public:
  constexpr OptionalBase() noexcept {}

  // Copy ctor.
  constexpr OptionalBase(const OptionalBase& other) noexcept {
    if (!other.storage_.is_null_)
      Init(other.storage_.value_);
  }

  // TODO: note for noexcept.
  constexpr OptionalBase(OptionalBase&& other) noexcept {
    if (!other.storage_.is_null_)
      Init(std::move(other.storage_.value_));
  }

  template <typename... Args>
  constexpr OptionalBase(base::in_place_t, Args&&... args)
      : storage_(base::in_place, std::forward<Args>(args)...) {}

  ~OptionalBase() = default;

  OptionalBase& operator=(const OptionalBase& other) {
    if (other.storage_.is_null_) {
      FreeIfNeeded();
      return *this;
    }

    InitOrAssign(other.storage_.value_);
    return *this;
  }

  // TODO comment noexcept.
  OptionalBase& operator=(OptionalBase&& other) noexcept {
    if (other.storage_.is_null_) {
      FreeIfNeeded();
      return *this;
    }

    InitOrAssign(std::move(other.storage_.value_));
    return *this;
  }

 protected:
  template <class... Args>
  constexpr void Init(Args&&... args) {
    DCHECK(storage_.is_null_);
    new (&storage_.value_) T(std::forward<Args>(args)...);
    storage_.is_null_ = false;
  }

  template <typename U>
  void InitOrAssign(U&& value) {
    if (storage_.is_null_)
      Init(std::forward<U>(value));
    else
      storage_.value_ = std::forward<U>(value);
  }

  void FreeIfNeeded() {
    if (storage_.is_null_)
      return;
    storage_.value_.~T();
    storage_.is_null_ = true;
  }

  internal::OptionalStorage<T> storage_;
};

// TODO comment.
template <bool allowed>
struct CopyConstructible {};

template <>
struct CopyConstructible<false> {
  CopyConstructible() noexcept = default;
  CopyConstructible(const CopyConstructible&) noexcept = delete;
  CopyConstructible(CopyConstructible&&) noexcept = default;
  CopyConstructible& operator=(const CopyConstructible&) noexcept = default;
  CopyConstructible& operator=(CopyConstructible&&) noexcept = default;
};

template <bool allowed>
struct MoveConstructible {};

template <>
struct MoveConstructible<false> {
  MoveConstructible() noexcept = default;
  MoveConstructible(const MoveConstructible&) noexcept = default;
  MoveConstructible(MoveConstructible&&) noexcept = delete;
  MoveConstructible& operator=(const MoveConstructible&) noexcept = default;
  MoveConstructible& operator=(MoveConstructible&&) noexcept = default;
};

template <bool allowed>
struct CopyAssignable {};

template <>
struct CopyAssignable<false> {
  CopyAssignable() noexcept = default;
  CopyAssignable(const CopyAssignable&) noexcept = default;
  CopyAssignable(CopyAssignable&&) noexcept = default;
  CopyAssignable& operator=(const CopyAssignable&) noexcept = delete;
  CopyAssignable& operator=(CopyAssignable&&) noexcept = default;
};

template <bool allowed>
struct MoveAssignable {};

template <>
struct MoveAssignable<false> {
  MoveAssignable() noexcept = default;
  MoveAssignable(const MoveAssignable&) noexcept = default;
  MoveAssignable(MoveAssignable&&) noexcept = default;
  MoveAssignable& operator=(const MoveAssignable&) noexcept = default;
  MoveAssignable& operator=(MoveAssignable&&) noexcept = delete;
};

}  // namespace internal

// base::Optional is a Chromium version of the C++17 optional class:
// std::optional documentation:
// http://en.cppreference.com/w/cpp/utility/optional
// Chromium documentation:
// https://chromium.googlesource.com/chromium/src/+/master/docs/optional.md
//
// These are the differences between the specification and the implementation:
// - The constructor and emplace method using initializer_list are not
//   implemented because 'initializer_list' is banned from Chromium.
// - 'constexpr' might be missing in some places for reasons specified locally.
// - No exceptions are thrown, because they are banned from Chromium.
// - All the non-members are in the 'base' namespace instead of 'std'.
template <typename T>
class Optional
    : public internal::OptionalBase<T>,
      public internal::CopyConstructible<std::is_copy_constructible<T>::value>,
      public internal::MoveConstructible<std::is_move_constructible<T>::value>,
      public internal::CopyAssignable<std::is_copy_constructible<T>::value &&
                                      std::is_copy_assignable<T>::value>,
      public internal::MoveAssignable<std::is_move_constructible<T>::value &&
                                      std::is_move_assignable<T>::value> {
 public:
  using value_type = T;

  // Inherit constructors.
  using internal::OptionalBase<T>::OptionalBase;

  constexpr Optional() noexcept = default;

  constexpr Optional(base::nullopt_t) noexcept {}

  // TODO comment.
  template<
    class U,
    std::enable_if_t<
      std::is_constructible<T, const U&>::value &&
      internal::IsOptionalConvertible<T, U>::value &&
      std::is_convertible<const U&, T>::value, bool> = false>
  Optional(const Optional<U>& other) {
    if (!other.storage_.is_null_)
      Init(*other);
  }

  template<
    class U,
    std::enable_if_t<
      std::is_constructible<T, const U&>::value &&
      internal::IsOptionalConvertible<T, U>::value &&
      !std::is_convertible<const U&, T>::value, bool> = false>
  explicit Optional(const Optional<U>& other) {
    if (!other.storage_.is_null_)
      Init(*other);
  }

  template<
    class U,
    std::enable_if_t<
      std::is_constructible<T, U&&>::value &&
      internal::IsOptionalConvertible<T, U>::value &&
      std::is_convertible<U&&, T>::value, bool> = false>
  Optional(Optional<U>&& other) {
    if (!other.storage_.is_null_)
      Init(std::move(*other));
  }

  template<
    class U,
    std::enable_if_t<
      std::is_constructible<T, U&&>::value &&
      internal::IsOptionalConvertible<T, U>::value &&
      !std::is_convertible<U&&, T>::value, bool> = false>
  explicit Optional(Optional<U>&& other) {
    if (!other.storage_.is_null_)
      Init(std::move(*other));
  }

  template <
    class... Args,
    std::enable_if_t<
      std::is_constructible<T, Args...>::value, bool> = false>
  constexpr explicit Optional(base::in_place_t, Args&&... args)
      : internal::OptionalBase<T>(base::in_place, std::forward<Args>(args)...) {}

  template <
    class U = value_type,
    std::enable_if_t<
      std::is_constructible<T, U&&>::value &&
      !std::is_same<base::in_place_t, std::decay_t<U>>::value &&
      !std::is_same<Optional<T>, std::decay_t<U>>::value &&
      std::is_convertible<U&&, T>::value, bool> = false>
  constexpr Optional(U&& value)
  : internal::OptionalBase<T>(base::in_place, std::forward<U>(value)) {}

  template <
    class U = value_type,
    std::enable_if_t<
      std::is_constructible<T, U&&>::value &&
      !std::is_same<base::in_place_t, std::decay_t<U>>::value &&
      !std::is_same<Optional<T>, std::decay_t<U>>::value &&
      !std::is_convertible<U&&, T>::value, bool> = false>
  explicit constexpr Optional(U&& value)
  : internal::OptionalBase<T>(base::in_place, std::forward<U>(value)) {}

  ~Optional() = default;

  Optional& operator=(base::nullopt_t) {
    FreeIfNeeded();
    return *this;
  }

  template <class U>
  std::enable_if_t<
    !std::is_same<std::decay_t<U>, Optional<T>>::value &&
    std::is_constructible<T, U>::value &&
    std::is_assignable<T&, U>::value &&
    (!std::is_scalar<T>::value ||
     !std::is_same<std::decay_t<U>, T>::value), Optional&>
  operator=(U&& value) {
    InitOrAssign(std::forward<U>(value));
    return *this;
  }

  template <class U>
  std::enable_if_t<
    !internal::IsOptionalAssignable<T, U>::value &&
    std::is_constructible<T, const U&>::value &&
    std::is_assignable<T&, const U&>::value, Optional&>
  operator=(const Optional<U>& other) {
    if (other.storage_.is_null_) {
      FreeIfNeeded();
      return *this;
    }

    InitOrAssign(other.value());
    return *this;
  }

  template <class U>
  std::enable_if_t<
    !internal::IsOptionalAssignable<T, U>::value &&
    std::is_constructible<T, U>::value &&
    std::is_assignable<T&, U>::value, Optional&>
  operator=(Optional<U>&& other) {
    if (other.storage_.is_null_) {
      FreeIfNeeded();
      return *this;
    }

    InitOrAssign(std::move(*other));
    return *this;
  }

  constexpr const T* operator->() const {
    DCHECK(!storage_.is_null_);
    return &value();
  }

  constexpr T* operator->() {
    DCHECK(!storage_.is_null_);
    return &value();
  }

  constexpr const T& operator*() const& { return value(); }

  constexpr T& operator*() & { return value(); }

  constexpr const T&& operator*() const&& { return std::move(value()); }

  constexpr T&& operator*() && { return std::move(value()); }

  constexpr explicit operator bool() const noexcept { return !storage_.is_null_; }

  constexpr bool has_value() const noexcept { return !storage_.is_null_; }

  constexpr T& value() & {
    DCHECK(!storage_.is_null_);
    return storage_.value_;
  }

  constexpr const T& value() const & {
    DCHECK(!storage_.is_null_);
    return storage_.value_;
  }

  constexpr T&& value() && {
    DCHECK(!storage_.is_null_);
    return std::move(storage_.value_);
  }

  constexpr const T&& value() const && {
    DCHECK(!storage_.is_null_);
    return std::move(storage_.value_);
  }

  template <class U>
  constexpr T value_or(U&& default_value) const& {
    // TODO(mlamouri): add the following assert when possible:
    // static_assert(std::is_copy_constructible<T>::value,
    //               "T must be copy constructible");
    static_assert(std::is_convertible<U, T>::value,
                  "U must be convertible to T");
    return storage_.is_null_ ? static_cast<T>(std::forward<U>(default_value))
                             : value();
  }

  template <class U>
  constexpr T value_or(U&& default_value) && {
    // TODO(mlamouri): add the following assert when possible:
    // static_assert(std::is_move_constructible<T>::value,
    //               "T must be move constructible");
    static_assert(std::is_convertible<U, T>::value,
                  "U must be convertible to T");
    return storage_.is_null_ ? static_cast<T>(std::forward<U>(default_value))
                             : std::move(value());
  }

  // TODO noexcept
  void swap(Optional& other) noexcept {
    if (storage_.is_null_ && other.storage_.is_null_)
      return;

    if (storage_.is_null_ != other.storage_.is_null_) {
      if (storage_.is_null_) {
        Init(std::move(other.storage_.value_));
        other.FreeIfNeeded();
      } else {
        other.Init(std::move(storage_.value_));
        FreeIfNeeded();
      }
      return;
    }

    DCHECK(!storage_.is_null_ && !other.storage_.is_null_);
    using std::swap;
    swap(**this, *other);
  }

  void reset() noexcept {
    FreeIfNeeded();
  }

  template <class... Args>
  T& emplace(Args&&... args) {
    FreeIfNeeded();
    Init(std::forward<Args>(args)...);
    return value();
  }

 private:
  // TODO comment.
  using internal::OptionalBase<T>::Init;
  using internal::OptionalBase<T>::InitOrAssign;
  using internal::OptionalBase<T>::FreeIfNeeded;
  using internal::OptionalBase<T>::storage_;
};

// TODO comment.
template <class T, class U>
constexpr bool operator==(const Optional<T>& lhs, const Optional<U>& rhs) {
  if (lhs.has_value() != rhs.has_value())
    return false;
  if (!lhs.has_value())
    return true;
  return *lhs == *rhs;
}

template <class T, class U>
constexpr bool operator!=(const Optional<T>& lhs, const Optional<U>& rhs) {
  if (lhs.has_value() != rhs.has_value())
    return true;
  if (!lhs.has_value())
    return false;
  return *lhs != *rhs;
}

template <class T, class U>
constexpr bool operator<(const Optional<T>& lhs, const Optional<U>& rhs) {
  if (!rhs.has_value())
    return false;
  if (!lhs.has_value())
    return true;
  return *lhs < *rhs;
}

template <class T, class U>
constexpr bool operator<=(const Optional<T>& lhs, const Optional<U>& rhs) {
  if (!lhs.has_value())
    return true;
  if (!rhs.has_value())
    return false;
  return *lhs <= *rhs;
}

template <class T, class U>
constexpr bool operator>(const Optional<T>& lhs, const Optional<U>& rhs) {
  if (!lhs.has_value())
    return false;
  if (!rhs.has_value())
    return true;
  return *lhs > *rhs;
}

template <class T, class U>
constexpr bool operator>=(const Optional<T>& lhs, const Optional<U>& rhs) {
  if (!rhs.has_value())
    return true;
  if (!lhs.has_value())
    return false;
  return *lhs >= *rhs;
}

template <class T>
constexpr bool operator==(const Optional<T>& opt, base::nullopt_t) {
  return !opt;
}

template <class T>
constexpr bool operator==(base::nullopt_t, const Optional<T>& opt) {
  return !opt;
}

template <class T>
constexpr bool operator!=(const Optional<T>& opt, base::nullopt_t) {
  return opt.has_value();
}

template <class T>
constexpr bool operator!=(base::nullopt_t, const Optional<T>& opt) {
  return opt.has_value();
}

template <class T>
constexpr bool operator<(const Optional<T>& opt, base::nullopt_t) {
  return false;
}

template <class T>
constexpr bool operator<(base::nullopt_t, const Optional<T>& opt) {
  return opt.has_value();
}

template <class T>
constexpr bool operator<=(const Optional<T>& opt, base::nullopt_t) {
  return !opt;
}

template <class T>
constexpr bool operator<=(base::nullopt_t, const Optional<T>& opt) {
  return true;
}

template <class T>
constexpr bool operator>(const Optional<T>& opt, base::nullopt_t) {
  return opt.has_value();
}

template <class T>
constexpr bool operator>(base::nullopt_t, const Optional<T>& opt) {
  return false;
}

template <class T>
constexpr bool operator>=(const Optional<T>& opt, base::nullopt_t) {
  return true;
}

template <class T>
constexpr bool operator>=(base::nullopt_t, const Optional<T>& opt) {
  return !opt;
}

template <class T, class U>
constexpr bool operator==(const Optional<T>& opt, const U& value) {
  return opt.has_value() ? *opt == value : false;
}

template <class T, class U>
constexpr bool operator==(const U& value, const Optional<T>& opt) {
  return opt.has_value() ? value == *opt : false;
}

template <class T, class U>
constexpr bool operator!=(const Optional<T>& opt, const U& value) {
  return opt.has_value() ? *opt != value : true;
}

template <class T, class U>
constexpr bool operator!=(const U& value, const Optional<T>& opt) {
  return opt.has_value() ? value != *opt : true;
}

template <class T, class U>
constexpr bool operator<(const Optional<T>& opt, const U& value) {
  return opt.has_value() ? *opt < value : true;
}

template <class T, class U>
constexpr bool operator<(const U& value, const Optional<T>& opt) {
  return opt.has_value() ? value < *opt : false;
}

template <class T, class U>
constexpr bool operator<=(const Optional<T>& opt, const U& value) {
  return opt.has_value() ? *opt <= value : true;
}

template <class T, class U>
constexpr bool operator<=(const U& value, const Optional<T>& opt) {
  return opt.has_value() ? value <= *opt : false;
}

template <class T, class U>
constexpr bool operator>(const Optional<T>& opt, const U& value) {
  return opt.has_value() ? *opt > value : false;
}

template <class T, class U>
constexpr bool operator>(const U& value, const Optional<T>& opt) {
  return opt.has_value() ? value > *opt : true;
}

template <class T, class U>
constexpr bool operator>=(const Optional<T>& opt, const U& value) {
  return opt.has_value() ? *opt >= value : false;
}

template <class T, class U>
constexpr bool operator>=(const U& value, const Optional<T>& opt) {
  return opt.has_value() ? value >= *opt : true;
}

template <class T>
constexpr Optional<std::decay_t<T>> make_optional(T&& value) {
  return {std::forward<T>(value)};
}

template <class T, class... Args>
constexpr Optional<T> make_optional(Args&&... args) {
  return {base::in_place, std::forward<Args>(args)...};
}

template <class T>
void swap(Optional<T>& lhs, Optional<T>& rhs) noexcept {
  lhs.swap(rhs);
}

}  // namespace base

namespace std {

template <class T>
struct hash<base::Optional<T>> {
  size_t operator()(const base::Optional<T>& opt) const {
    return opt == base::nullopt ? 0 : std::hash<T>()(*opt);
  }
};

}  // namespace std

#endif  // BASE_OPTIONAL_H_
