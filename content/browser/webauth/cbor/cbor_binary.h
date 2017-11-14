// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#ifndef CONTENT_BROWSER_WEBAUTH_CBOR_CBOR_BINARY_H_
#define CONTENT_BROWSER_WEBAUTH_CBOR_CBOR_BINARY_H_

namespace content {
namespace authenticator_impl {

// Mask selecting the low-order 5 bits of the "initial byte", which is where
// the additional information is encoded.
static constexpr uint8_t kAdditionalInformationDataMask = 0x1F;
// Mask selecting the high-order 3 bits of the "initial byte", which indicates
// the major type of the encoded value.
static constexpr uint8_t kMajorTypeDataMask = 0xE0;
// Indicates number of bits to shift to the right to read major type from first
// CBOR byte.
static constexpr uint8_t kMajorTypeBitShift = 5u;
// Indicates the integer is in the following byte.
static constexpr uint8_t kAdditionalInformation1Byte = 24u;
// Indicates the integer is in the next 2 bytes.
static constexpr uint8_t kAdditionalInformation2Bytes = 25u;
// Indicates the integer is in the next 4 bytes.
static constexpr uint8_t kAdditionalInformation4Bytes = 26u;
// Indicates the integer is in the next 8 bytes.
static constexpr uint8_t kAdditionalInformation8Bytes = 27u;

}  // namespace authenticator_impl
}  // namespace content

#endif  // CONTENT_BROWSER_WEBAUTH_CBOR_CBOR_BINARY_H_
