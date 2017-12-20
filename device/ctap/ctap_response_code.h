// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CTAP_CTAP_RESPONSE_CODE_H_
#define DEVICE_CTAP_CTAP_RESPONSE_CODE_H_

#include <stdint.h>
#include <vector>

namespace device {

// CTAP protocol device response code, as specified in
// https://fidoalliance.org/specs/fido-v2.0-rd-20170927/fido-client-to-
// authenticator-protocol-v2.0-rd-20170927.html#authenticator-api .
enum class CTAPResponseCode : uint8_t {
  kSuccess = 0x00,
  kCtap1ErrInvalidCommand = 0X01,
  kCtap1ErrInvalidParameter = 0X02,
  kCtap1ErrInvalidLength = 0X03,
  kCtap1ErrInvalidSeq = 0X04,
  kCtap1ErrTimeout = 0X05,
  kCtap1ErrChannelBusy = 0X06,
  kCtap1ErrLockRequired = 0X0A,
  kCtap1ErrInvalidChannel = 0X0B,
  kCtap2ErrCBORParsing = 0X10,
  kCtap2ErrUnexpectedType = 0X11,
  kCtap2ErrInvalidCBOR = 0X12,
  kCtap2ErrInvalidCBORType = 0X13,
  kCtap2ErrMissingParameter = 0X14,
  kCtap2ErrLimitExceeded = 0X15,
  kCtap2ErrUnsupportedExtension = 0X16,
  kCtap2ErrTooManyElements = 0X17,
  kCtap2ErrExtensionNotSupported = 0X18,
  kCtap2ErrCredentialExcluded = 0X19,
  kCtap2ErrCredentialNotValid = 0X20,
  kCtap2ErrProcesssing = 0X21,
  kCtap2ErrInvalidCredential = 0X22,
  kCtap2ErrUserActionPending = 0X23,
  kCtap2ErrOperationPending = 0X24,
  kCtap2ErrNoOperations = 0X25,
  kCtap2ErrUnsupportedAlgorithms = 0X26,
  kCtap2ErrOperationDenied = 0X27,
  kCtap2ErrKeyStoreFull = 0X28,
  kCtap2ErrNotBusy = 0X29,
  kCtap2ErrNoOperationPending = 0X2A,
  kCtap2ErrUnsupportedOption = 0X2B,
  kCtap2ErrInvalidOption = 0X2C,
  kCtap2ErrKeepAliveCancel = 0X2D,
  kCtap2ErrNoCredentials = 0X2E,
  kCtap2ErrUserActionTimeout = 0X2F,
  kCtap2ErrNotAllowed = 0X30,
  kCtap2ErrPinInvalid = 0X31,
  kCtap2ErrPinBlocked = 0X32,
  kCtap2ErrPinAuthInvalid = 0X33,
  kCtap2ErrPinAuthBlocked = 0X34,
  kCtap2ErrPinNotSet = 0X35,
  kCtap2ErrPinRequired = 0X36,
  kCtap2ErrPinPolicyViolation = 0X37,
  kCtap2ErrPinTokenExpired = 0X38,
  kCtap2ErrRequestTooLarge = 0X39,
  kCtap2ErrOther = 0X7F,
  kCtap2ErrSpecLast = 0XDF,
  kCtap2ErrExtensionFirst = 0XE0,
  kCtap2ErrExtensionLast = 0XEF,
  kCtap2ErrVendorFirst = 0XF0,
  kCtap2ErrVendorLast = 0XFF
};

constexpr CTAPResponseCode RESPONSE_CODE_LIST[] = {
    CTAPResponseCode::kSuccess,
    CTAPResponseCode::kCtap1ErrInvalidCommand,
    CTAPResponseCode::kCtap1ErrInvalidParameter,
    CTAPResponseCode::kCtap1ErrInvalidLength,
    CTAPResponseCode::kCtap1ErrInvalidSeq,
    CTAPResponseCode::kCtap1ErrTimeout,
    CTAPResponseCode::kCtap1ErrChannelBusy,
    CTAPResponseCode::kCtap1ErrLockRequired,
    CTAPResponseCode::kCtap1ErrInvalidChannel,
    CTAPResponseCode::kCtap2ErrCBORParsing,
    CTAPResponseCode::kCtap2ErrUnexpectedType,
    CTAPResponseCode::kCtap2ErrInvalidCBOR,
    CTAPResponseCode::kCtap2ErrInvalidCBORType,
    CTAPResponseCode::kCtap2ErrMissingParameter,
    CTAPResponseCode::kCtap2ErrLimitExceeded,
    CTAPResponseCode::kCtap2ErrUnsupportedExtension,
    CTAPResponseCode::kCtap2ErrTooManyElements,
    CTAPResponseCode::kCtap2ErrExtensionNotSupported,
    CTAPResponseCode::kCtap2ErrCredentialExcluded,
    CTAPResponseCode::kCtap2ErrCredentialNotValid,
    CTAPResponseCode::kCtap2ErrProcesssing,
    CTAPResponseCode::kCtap2ErrInvalidCredential,
    CTAPResponseCode::kCtap2ErrUserActionPending,
    CTAPResponseCode::kCtap2ErrOperationPending,
    CTAPResponseCode::kCtap2ErrNoOperations,
    CTAPResponseCode::kCtap2ErrUnsupportedAlgorithms,
    CTAPResponseCode::kCtap2ErrOperationDenied,
    CTAPResponseCode::kCtap2ErrKeyStoreFull,
    CTAPResponseCode::kCtap2ErrNotBusy,
    CTAPResponseCode::kCtap2ErrNoOperationPending,
    CTAPResponseCode::kCtap2ErrUnsupportedOption,
    CTAPResponseCode::kCtap2ErrInvalidOption,
    CTAPResponseCode::kCtap2ErrKeepAliveCancel,
    CTAPResponseCode::kCtap2ErrNoCredentials,
    CTAPResponseCode::kCtap2ErrUserActionTimeout,
    CTAPResponseCode::kCtap2ErrNotAllowed,
    CTAPResponseCode::kCtap2ErrPinInvalid,
    CTAPResponseCode::kCtap2ErrPinBlocked,
    CTAPResponseCode::kCtap2ErrPinAuthInvalid,
    CTAPResponseCode::kCtap2ErrPinAuthBlocked,
    CTAPResponseCode::kCtap2ErrPinNotSet,
    CTAPResponseCode::kCtap2ErrPinRequired,
    CTAPResponseCode::kCtap2ErrPinPolicyViolation,
    CTAPResponseCode::kCtap2ErrPinTokenExpired,
    CTAPResponseCode::kCtap2ErrRequestTooLarge,
    CTAPResponseCode::kCtap2ErrOther,
    CTAPResponseCode::kCtap2ErrSpecLast,
    CTAPResponseCode::kCtap2ErrExtensionFirst,
    CTAPResponseCode::kCtap2ErrExtensionLast,
    CTAPResponseCode::kCtap2ErrVendorFirst,
    CTAPResponseCode::kCtap2ErrVendorLast};

}  // namespace device

#endif  // DEVICE_CTAP_CTAP_RESPONSE_CODE_H_
