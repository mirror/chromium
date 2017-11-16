// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MixedContentContextTypeStructTraits_h
#define MixedContentContextTypeStructTraits_h

#include "mojo/public/cpp/bindings/enum_traits.h"
#include "third_party/WebKit/public/platform/WebMixedContentContextType.h"
#include "third_party/WebKit/public/platform/mixed_content_context_type.mojom-shared.h"

namespace mojo {

template <>
struct EnumTraits<::blink::mojom::MixedContentContextType,
                  ::blink::WebMixedContentContextType> {
  static ::blink::mojom::MixedContentContextType ToMojom(
      ::blink::WebMixedContentContextType input) {
    switch (input) {
      case ::blink::WebMixedContentContextType::kNotMixedContent:
        return ::blink::mojom::MixedContentContextType::kNotMixedContent;
      case ::blink::WebMixedContentContextType::kBlockable:
        return ::blink::mojom::MixedContentContextType::kBlockable;
      case ::blink::WebMixedContentContextType::kOptionallyBlockable:
        return ::blink::mojom::MixedContentContextType::kOptionallyBlockable;
      case ::blink::WebMixedContentContextType::kShouldBeBlockable:
        return ::blink::mojom::MixedContentContextType::kShouldBeBlockable;
    }
    NOTREACHED();
    return ::blink::mojom::MixedContentContextType::kNotMixedContent;
  }

  static bool FromMojom(::blink::mojom::MixedContentContextType input,
                        ::blink::WebMixedContentContextType* output) {
    switch (input) {
      case ::blink::mojom::MixedContentContextType::kNotMixedContent:
        *output = ::blink::WebMixedContentContextType::kNotMixedContent;
        return true;
      case ::blink::mojom::MixedContentContextType::kBlockable:
        *output = ::blink::WebMixedContentContextType::kBlockable;
        return true;
      case ::blink::mojom::MixedContentContextType::kOptionallyBlockable:
        *output = ::blink::WebMixedContentContextType::kOptionallyBlockable;
        return true;
      case ::blink::mojom::MixedContentContextType::kShouldBeBlockable:
        *output = ::blink::WebMixedContentContextType::kShouldBeBlockable;
        return true;
    }
    NOTREACHED();
    return false;
  }
};

}  // namespace mojo

#endif  // MixedContentContextTypeStructTraits_h
