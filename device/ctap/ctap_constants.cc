// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/ctap_constants.h"

namespace device {

namespace constants {

const std::vector<CTAPDeviceResponseCode> kResponseCodeList = {
    CTAPDeviceResponseCode::kSuccess,
    CTAPDeviceResponseCode::kCtap1ErrInvalidCommand,
    CTAPDeviceResponseCode::kCtap1ErrInvalidParameter,
    CTAPDeviceResponseCode::kCtap1ErrInvalidLength,
    CTAPDeviceResponseCode::kCtap1ErrInvalidSeq,
    CTAPDeviceResponseCode::kCtap1ErrTimeout,
    CTAPDeviceResponseCode::kCtap1ErrChannelBusy,
    CTAPDeviceResponseCode::kCtap1ErrLockRequired,
    CTAPDeviceResponseCode::kCtap1ErrInvalidChannel,
    CTAPDeviceResponseCode::kCtap2ErrCBORParsing,
    CTAPDeviceResponseCode::kCtap2ErrUnexpectedType,
    CTAPDeviceResponseCode::kCtap2ErrInvalidCBOR,
    CTAPDeviceResponseCode::kCtap2ErrInvalidCBORType,
    CTAPDeviceResponseCode::kCtap2ErrMissingParameter,
    CTAPDeviceResponseCode::kCtap2ErrLimitExceeded,
    CTAPDeviceResponseCode::kCtap2ErrUnsupportedExtension,
    CTAPDeviceResponseCode::kCtap2ErrTooManyElements,
    CTAPDeviceResponseCode::kCtap2ErrExtensionNotSupported,
    CTAPDeviceResponseCode::kCtap2ErrCredentialExcluded,
    CTAPDeviceResponseCode::kCtap2ErrCredentialNotValid,
    CTAPDeviceResponseCode::kCtap2ErrProcesssing,
    CTAPDeviceResponseCode::kCtap2ErrInvalidCredential,
    CTAPDeviceResponseCode::kCtap2ErrUserActionPending,
    CTAPDeviceResponseCode::kCtap2ErrOperationPending,
    CTAPDeviceResponseCode::kCtap2ErrNoOperations,
    CTAPDeviceResponseCode::kCtap2ErrUnsupportedAlgorithms,
    CTAPDeviceResponseCode::kCtap2ErrOperationDenied,
    CTAPDeviceResponseCode::kCtap2ErrKeyStoreFull,
    CTAPDeviceResponseCode::kCtap2ErrNotBusy,
    CTAPDeviceResponseCode::kCtap2ErrNoOperationPending,
    CTAPDeviceResponseCode::kCtap2ErrUnsupportedOption,
    CTAPDeviceResponseCode::kCtap2ErrInvalidOption,
    CTAPDeviceResponseCode::kCtap2ErrKeepAliveCancel,
    CTAPDeviceResponseCode::kCtap2ErrNoCredentials,
    CTAPDeviceResponseCode::kCtap2ErrUserActionTimeout,
    CTAPDeviceResponseCode::kCtap2ErrNotAllowed,
    CTAPDeviceResponseCode::kCtap2ErrPinInvalid,
    CTAPDeviceResponseCode::kCtap2ErrPinBlocked,
    CTAPDeviceResponseCode::kCtap2ErrPinAuthInvalid,
    CTAPDeviceResponseCode::kCtap2ErrPinAuthBlocked,
    CTAPDeviceResponseCode::kCtap2ErrPinNotSet,
    CTAPDeviceResponseCode::kCtap2ErrPinRequired,
    CTAPDeviceResponseCode::kCtap2ErrPinPolicyViolation,
    CTAPDeviceResponseCode::kCtap2ErrPinTokenExpired,
    CTAPDeviceResponseCode::kCtap2ErrRequestTooLarge,
    CTAPDeviceResponseCode::kCtap2ErrOther,
    CTAPDeviceResponseCode::kCtap2ErrSpecLast,
    CTAPDeviceResponseCode::kCtap2ErrExtensionFirst,
    CTAPDeviceResponseCode::kCtap2ErrExtensionLast,
    CTAPDeviceResponseCode::kCtap2ErrVendorFirst,
    CTAPDeviceResponseCode::kCtap2ErrVendorLast};

}  // namespace constants

}  // namespace device
