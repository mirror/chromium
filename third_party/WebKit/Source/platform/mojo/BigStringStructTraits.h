// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BigStringStructTraits_h
#define BigStringStructTraits_h

#include "mojo/common/big_string.mojom-blink.h"
#include "mojo/public/cpp/bindings/struct_traits.h"
#include "platform/PlatformExport.h"

namespace mojo_base {
class BigBuffer;
}

namespace mojo {

template <>
struct PLATFORM_EXPORT
    StructTraits<common::mojom::BigStringDataView, WTF::String> {
  static bool IsNull(const WTF::String& input) { return input.IsNull(); }
  static void SetToNull(WTF::String* output) { *output = WTF::String(); }

  static void* SetUpContext(const WTF::String& input);
  static void TearDownContext(const WTF::String& input, void* context);

  static mojo_base::BigBuffer data(const WTF::String& input, void* context);
  static bool Read(common::mojom::BigStringDataView, WTF::String* out);
};

}  // namespace mojo

#endif  // BigStringStructTraits_h
