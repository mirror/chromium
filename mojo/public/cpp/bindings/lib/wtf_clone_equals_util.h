// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_BINDINGS_LIB_WTF_CLONE_EQUALS_UTIL_H_
#define MOJO_PUBLIC_CPP_BINDINGS_LIB_WTF_CLONE_EQUALS_UTIL_H_

#include <type_traits>

#include "mojo/public/cpp/bindings/clone_traits.h"
#include "mojo/public/cpp/bindings/equals_traits.h"
#include "third_party/WebKit/Source/platform/wtf/HashMap.h"
#include "third_party/WebKit/Source/platform/wtf/Optional.h"
#include "third_party/WebKit/Source/platform/wtf/Vector.h"
#include "third_party/WebKit/Source/platform/wtf/text/WTFString.h"

namespace mojo {

template <typename T>
struct CloneTraits<WTF::Vector<T>, false> {
  static WTF::Vector<T> Clone(const WTF::Vector<T>& input) {
    WTF::Vector<T> result;
    result.ReserveCapacity(input.size());
    for (const auto& element : input)
      result.push_back(mojo::Clone(element));

    return result;
  }
};

template <typename K, typename V>
struct CloneTraits<WTF::HashMap<K, V>, false> {
  static WTF::HashMap<K, V> Clone(const WTF::HashMap<K, V>& input) {
    WTF::HashMap<K, V> result;
    auto input_end = input.end();
    for (auto it = input.begin(); it != input_end; ++it)
      result.add(mojo::Clone(it->key), mojo::Clone(it->value));
    return result;
  }
};

}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_BINDINGS_LIB_WTF_CLONE_EQUALS_UTIL_H_
