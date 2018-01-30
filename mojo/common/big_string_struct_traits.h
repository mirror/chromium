// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_COMMON_BIG_STRING_STRUCT_TRAITS_H_
#define MOJO_COMMON_BIG_STRING_STRUCT_TRAITS_H_

#include <string>

#include "mojo/common/big_string.mojom-shared.h"
#include "mojo/public/cpp/base/big_buffer.h"
#include "mojo/public/cpp/bindings/struct_traits.h"

namespace mojo {

template <>
struct StructTraits<common::mojom::BigStringDataView, std::string> {
  static mojo_base::BigBuffer data(const std::string& str);

  static bool Read(common::mojom::BigStringDataView data, std::string* out);
};

}  // namespace mojo

#endif  // MOJO_COMMON_BIG_STRING_STRUCT_TRAITS_H_
