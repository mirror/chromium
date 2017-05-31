// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZUCCHINI_RANGES_FUNCTIONAL_H_
#define ZUCCHINI_RANGES_FUNCTIONAL_H_

#include <functional>
#include <utility>

namespace zucchini {
namespace ranges {

template <class T>
T as_function(T&& t) {
  return std::forward<T>(t);
}

template <class R, class T>
auto as_function(R T::*pm) -> decltype(std::mem_fn(pm)) {
  return std::mem_fn(pm);
}

template <class T>
using as_function_t = decltype(as_function(std::declval<T>()));

struct identity {
  template <class T>
  T&& operator()(T&& t) const {
    return std::forward<T>(t);
  }
};

struct plus {
  template <class T, class U>
  constexpr auto operator()(T&& lhs, U&& rhs) const
      -> decltype(std::forward<T>(lhs) + std::forward<U>(rhs)) {
    return std::forward<T>(lhs) + std::forward<U>(rhs);
  }
};

struct minus {
  template <class T, class U>
  constexpr auto operator()(T&& lhs, U&& rhs) const
      -> decltype(std::forward<T>(lhs) - std::forward<U>(rhs)) {
    return std::forward<T>(lhs) - std::forward<U>(rhs);
  }
};

struct multiplies {
  template <class T, class U>
  constexpr auto operator()(T&& lhs, U&& rhs) const
      -> decltype(std::forward<T>(lhs) * std::forward<U>(rhs)) {
    return std::forward<T>(lhs) * std::forward<U>(rhs);
  }
};

struct divides {
  template <class T, class U>
  constexpr auto operator()(T&& lhs, U&& rhs) const
      -> decltype(std::forward<T>(lhs) / std::forward<U>(rhs)) {
    return std::forward<T>(lhs) / std::forward<U>(rhs);
  }
};

struct modulus {
  template <class T, class U>
  constexpr auto operator()(T&& lhs, U&& rhs) const
      -> decltype(std::forward<T>(lhs) % std::forward<U>(rhs)) {
    return std::forward<T>(lhs) % std::forward<U>(rhs);
  }
};

struct negate {
  template <class T>
  constexpr auto operator()(T&& arg) const -> decltype(-std::forward<T>(arg)) {
    return -std::forward<T>(arg);
  }
};

struct equal_to {
  template <class T, class U>
  constexpr auto operator()(T&& lhs, U&& rhs) const
      -> decltype(std::forward<T>(lhs) == std::forward<U>(rhs)) {
    return std::forward<T>(lhs) == std::forward<U>(rhs);
  }
};

struct not_equal_to {
  template <class T, class U>
  constexpr auto operator()(T&& lhs, U&& rhs) const
      -> decltype(std::forward<T>(lhs) != std::forward<U>(rhs)) {
    return std::forward<T>(lhs) != std::forward<U>(rhs);
  }
};

struct less {
  template <class T, class U>
  constexpr auto operator()(T&& lhs, U&& rhs) const
      -> decltype(std::forward<T>(lhs) < std::forward<U>(rhs)) {
    return std::forward<T>(lhs) < std::forward<U>(rhs);
  }
};

struct greater {
  template <class T, class U>
  constexpr auto operator()(T&& lhs, U&& rhs) const
      -> decltype(std::forward<T>(lhs) > std::forward<U>(rhs)) {
    return std::forward<T>(lhs) > std::forward<U>(rhs);
  }
};

struct less_equal {
  template <class T, class U>
  constexpr auto operator()(T&& lhs, U&& rhs) const
      -> decltype(std::forward<T>(lhs) <= std::forward<U>(rhs)) {
    return std::forward<T>(lhs) <= std::forward<U>(rhs);
  }
};

struct greater_equal {
  template <class T, class U>
  constexpr auto operator()(T&& lhs, U&& rhs) const
      -> decltype(std::forward<T>(lhs) >= std::forward<U>(rhs)) {
    return std::forward<T>(lhs) >= std::forward<U>(rhs);
  }
};

struct logical_and {
  template <class T, class U>
  constexpr auto operator()(T&& lhs, U&& rhs) const
      -> decltype(std::forward<T>(lhs) && std::forward<U>(rhs)) {
    return std::forward<T>(lhs) && std::forward<U>(rhs);
  }
};

struct logical_or {
  template <class T, class U>
  constexpr auto operator()(T&& lhs, U&& rhs) const
      -> decltype(std::forward<T>(lhs) || std::forward<U>(rhs)) {
    return std::forward<T>(lhs) || std::forward<U>(rhs);
  }
};

struct logical_not {
  template <class T, class U>
  constexpr auto operator()(T&& lhs) const -> decltype(!std::forward<T>(lhs)) {
    return !std::forward<T>(lhs);
  }
};

struct key {
  template <class T>
  constexpr auto operator()(T&& value) const
      -> decltype(std::forward<T>(value).first) {
    return std::forward<T>(value).first;
  }
};

struct value {
  template <class T>
  constexpr auto operator()(T&& value) const
      -> decltype(std::forward<T>(value).second) {
    return std::forward<T>(value).second;
  }
};

}  // namespace ranges
}  // namespace zucchini

#endif  // ZUCCHINI_RANGES_FUNCTIONAL_H_
