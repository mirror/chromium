// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SANDBOX_MAC_MOJO_SEATBELT_EXTENSION_TOKEN_STRUCT_TRAITS_H_
#define SANDBOX_MAC_MOJO_SEATBELT_EXTENSION_TOKEN_STRUCT_TRAITS_H_

#include <string>

#include "sandbox/mac/mojo/seatbelt_extension_token.mojom-shared.h"
#include "sandbox/mac/seatbelt_extension_token.h"

namespace mojo {

template <>
struct StructTraits<sandbox::mac::mojom::SeatbeltExtensionTokenDataView,
                    sandbox::SeatbeltExtensionToken> {
  static std::string token(const sandbox::SeatbeltExtensionToken& t) {
    return t.token();
  }
  static bool Read(sandbox::mac::mojom::SeatbeltExtensionTokenDataView data,
                   sandbox::SeatbeltExtensionToken* out) {
    std::string token;
    if (!data.ReadToken(&token))
      return false;

    out->set_token(token);
    return true;
  }
};

}  // namespace mojo

#endif  // SANDBOX_MAC_MOJO_SEATBELT_EXTENSION_TOKEN_STRUCT_TRAITS_H_
