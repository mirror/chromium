// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/mojo/KURLStructTraits.h"

#include "mojo/public/cpp/bindings/string_traits_wtf.h"

namespace mojo {

bool StructTraits<url::mojom::blink::Url::DataView, ::blink::KURL>::Read(
    url::mojom::blink::Url::DataView data,
    ::blink::KURL* out) {
  bool is_valid = data.is_valid();
  if (!is_valid) {
    *out = ::blink::KURL();
    return true;
  }

  WTF::String spec;
  if (!data.ReadSpec(&spec))
    return false;

  if (spec.length() > url::kMaxURLChars)
    return false;

  *out = ::blink::KURL(::blink::KURL(), spec);
  if (!spec.IsEmpty() && !out->IsValid())
    return false;

  return true;
}
}  // namespace mojo
