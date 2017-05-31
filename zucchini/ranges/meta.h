// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZUCCHINI_RANGES_META_H_
#define ZUCCHINI_RANGES_META_H_

#include <cstddef>
#include <type_traits>

namespace zucchini {
namespace ranges {
namespace meta {

template <bool B, class T = void>
using if_t = typename std::enable_if<B, T>::type;

template <bool B, class If, class Then>
using conditional_t = typename std::conditional<B, If, Then>::type;

namespace detail {

template <class Initial, template <class, class> class Fun, class... Types>
struct reduce_parser {};

template <class Initial,
          template <class, class> class Fun,
          class Tail,
          class... Types>
struct reduce_parser<Initial, Fun, Tail, Types...> {
  using previous_type = typename reduce_parser<Initial, Fun, Types...>::type;
  using type = typename Fun<previous_type, Tail>::type;
};

template <class Initial, template <class, class> class Fun>
struct reduce_parser<Initial, Fun> {
  using type = Initial;
};

template <class Enable, template <class> class Predicate, class... Types>
struct predicate_parser {};

template <template <class> class Predicate, class Tail, class... Types>
struct predicate_parser<if_t<!Predicate<Tail>::value>,
                        Predicate,
                        Tail,
                        Types...>
    : public predicate_parser<void, Predicate, Types...> {
  using found = typename predicate_parser<void, Predicate, Types...>::found;
  using ifound = std::true_type;
  using count = typename predicate_parser<void, Predicate, Types...>::count;
};

template <template <class> class Predicate, class Tail, class... Types>
struct predicate_parser<if_t<Predicate<Tail>::value>, Predicate, Tail, Types...>
    : public predicate_parser<void, Predicate, Types...> {
  using found = std::true_type;
  using ifound = typename predicate_parser<void, Predicate, Types...>::ifound;
  using count = std::integral_constant<
      size_t,
      predicate_parser<void, Predicate, Types...>::count::value + 1>;
  using index = std::integral_constant<size_t, sizeof...(Types) + 1>;
};

template <template <class> class Predicate>
struct predicate_parser<void, Predicate> {
  using found = std::false_type;
  using ifound = std::false_type;
  using count = std::integral_constant<size_t, 0>;
};

}  // namespace detail

template <class Tuple, class U>
struct index {};
template <template <class...> class Tuple, class... Types, class U>
struct index<Tuple<Types...>, U> {
  template <class V>
  struct is_same {
    static constexpr bool value = std::is_same<U, V>::value;
  };

  using type = std::integral_constant<
      size_t,
      sizeof...(Types) -
          detail::predicate_parser<void, is_same, Types...>::index::value>;
  static constexpr typename type::value_type value = type::value;
};

template <class Tuple, class U>
struct contains {};
template <template <class...> class Tuple, class... Types, class U>
struct contains<Tuple<Types...>, U> {
  template <class V>
  struct is_same {
    static constexpr bool value = std::is_same<U, V>::value;
  };
  using type =
      typename detail::predicate_parser<void, is_same, Types...>::found;
  static constexpr typename type::value_type value = type::value;
};

template <class Tuple, template <class> class Predicate>
struct any {};
template <template <class...> class Tuple,
          class... Types,
          template <class> class Predicate>
struct any<Tuple<Types...>, Predicate> {
  using type =
      typename detail::predicate_parser<void, Predicate, Types...>::found;
  static constexpr typename type::value_type value = type::value;
};

template <class Tuple, template <class> class Predicate>
struct none {};
template <template <class...> class Tuple,
          class... Types,
          template <class> class Predicate>
struct none<Tuple<Types...>, Predicate> {
  using type =
      typename std::conditional<!any<Tuple<Types...>, Predicate>::value,
                                std::true_type,
                                std::false_type>::type;
  static constexpr typename type::value_type value = type::value;
};

template <class Tuple, template <class> class Predicate>
struct all {};
template <template <class...> class Tuple,
          class... Types,
          template <class> class Predicate>
struct all<Tuple<Types...>, Predicate> {
  using type = typename std::conditional<
      !detail::predicate_parser<void, Predicate, Types...>::ifound::value,
      std::true_type,
      std::false_type>::type;
  static constexpr typename type::value_type value = type::value;
};

template <class Tuple, template <class> class Predicate>
struct count_if {};
template <template <class...> class Tuple,
          class... Types,
          template <class> class Predicate>
struct count_if<Tuple<Types...>, Predicate> {
  using type =
      typename detail::predicate_parser<void, Predicate, Types...>::count;
  static constexpr typename type::value_type value = type::value;
};

template <class Tuple, template <class> class Fun>
struct transform {};
template <template <class...> class Tuple,
          class... Types,
          template <class> class Fun>
struct transform<Tuple<Types...>, Fun> {
  using type = Tuple<typename Fun<Types>::type...>;
};
template <class Tuple, template <class> class Fun>
using transform_t = typename transform<Tuple, Fun>::type;

template <class Tuple, class Initial, template <class, class> class Fun>
struct reduce {};
template <template <class...> class Tuple,
          class... Types,
          class Initial,
          template <class, class> class Fun>
struct reduce<Tuple<Types...>, Initial, Fun> {
  using type = typename detail::reduce_parser<Initial, Fun, Types...>::type;
};
template <class Tuple, class Initial, template <class, class> class Fun>
using reduce_t = typename reduce<Tuple, Initial, Fun>::type;

}  // namespace meta
}  // namespace ranges
}  // namespace zucchini

#endif  // ZUCCHINI_RANGES_META_H_
