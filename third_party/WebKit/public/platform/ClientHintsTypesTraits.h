// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ClientHintsTypesTraits_h
#define ClientHintsTypesTraits_h

#include "WebClientHintsType.h"
#include "mojo/public/cpp/bindings/enum_traits.h"
#include "third_party/WebKit/public/platform/web_client_hints_types.mojom-shared.h"

namespace mojo {

// Ensure that the blink::WebClientHintsType enum values stay in sync with the
// blink::mojom::WebClientHintsType.
#define STATIC_ASSERT_ENUM(a, b)                            \
  static_assert(static_cast<int>(a) == static_cast<int>(b), \
                "mismatching enum : " #a)

STATIC_ASSERT_ENUM(
    ::blink::WebClientHintsType::kWebClientHintsTypeDeviceMemory,
    ::blink::mojom::WebClientHintsType::kWebClientHintsTypeDeviceMemory);

STATIC_ASSERT_ENUM(::blink::WebClientHintsType::kWebClientHintsTypeDpr,
                   ::blink::mojom::WebClientHintsType::kWebClientHintsTypeDpr);

STATIC_ASSERT_ENUM(
    ::blink::WebClientHintsType::kWebClientHintsTypeResourceWidth,
    ::blink::mojom::WebClientHintsType::kWebClientHintsTypeResourceWidth);

STATIC_ASSERT_ENUM(
    ::blink::WebClientHintsType::kWebClientHintsTypeViewportWidth,
    ::blink::mojom::WebClientHintsType::kWebClientHintsTypeViewportWidth);

STATIC_ASSERT_ENUM(::blink::WebClientHintsType::kWebClientHintsTypeLast,
                   ::blink::mojom::WebClientHintsType::kWebClientHintsTypeLast);

// Checks that fail if a new enum value is added but the static assert checks
// in this file are not updated.
STATIC_ASSERT_ENUM(
    ::blink::WebClientHintsType::kWebClientHintsTypeViewportWidth,
    ::blink::WebClientHintsType::kWebClientHintsTypeLast);

STATIC_ASSERT_ENUM(
    ::blink::mojom::WebClientHintsType::kWebClientHintsTypeViewportWidth,
    ::blink::mojom::WebClientHintsType::kWebClientHintsTypeLast);

}  // namespace mojo

#endif
