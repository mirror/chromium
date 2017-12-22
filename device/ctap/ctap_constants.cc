// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/ctap_constants.h"

namespace device {

namespace constants {

const std::vector<CTAPResponseCode> kResponseCodeList = {
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

}  // namespace constants

}  // namespace device
