// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_COMMON_QUOTA_QUOTA_STATUS_CODE_H_
#define THIRD_PARTY_WEBKIT_COMMON_QUOTA_QUOTA_STATUS_CODE_H_

#include "third_party/WebKit/common/common_export.h"

namespace blink {

enum QuotaStatusCode {
  kQuotaStatusOk = 0,
  kQuotaErrorNotSupported = 7,
  kQuotaErrorInvalidModification = 11,
  kQuotaErrorInvalidAccess = 13,
  kQuotaErrorAbort = 17,
  kQuotaStatusUnknown = -1,

  kQuotaStatusLast = kQuotaErrorAbort,
};

BLINK_COMMON_EXPORT const char* QuotaStatusCodeToString(QuotaStatusCode status);

}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_COMMON_QUOTA_QUOTA_STATUS_CODE_H_
