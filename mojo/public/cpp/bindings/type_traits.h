// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_BINDINGS_TYPE_TRAITS_H_
#define MOJO_PUBLIC_CPP_BINDINGS_TYPE_TRAITS_H_

#include <map>
#include <string>
#include <vector>

namespace mojo {

// Helper type to deduce how a specific type is used by generated code and
// interfaces.
template <typename T>
struct TypeTraits {
  // Indicates how |T| is passed as an input argument to Mojom method calls and
  // response callbacks.
  //
  // By default we assume by-value, but this is overridden by specializations
  // below and, in some cases, in generated code where typemapping is applied.
  using ParamType = T;
};

// Specializations for representations of some builtin mojom types.

template <>
struct TypeTraits<std::string> {
  using ParamType = const std::string&;
};

template <typename T>
struct TypeTraits<std::vector<T>> {
  using ParamType = const std::vector<T>&;
};

template <typename K, typename V>
struct TypeTraits<std::map<K, V>> {
  using ParamType = const std::map<K, V>&;
};

}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_BINDINGS_TYPE_TRAITS_H_
