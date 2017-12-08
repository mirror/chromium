/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebStorageQuotaError_h
#define WebStorageQuotaError_h

#include "third_party/WebKit/common/quota/quota_status_code.h"

namespace blink {

// The error code used for WebStorageQuota. Must match blink::QuotaStatusCode.
// TODO(sashab): Remove this class, use blink::QuotaStatusCode instead.
enum WebStorageQuotaError {
  kWebStorageQuotaErrorNotSupported = 7,
  kWebStorageQuotaErrorInvalidModification = 11,
  kWebStorageQuotaErrorInvalidAccess = 13,
  kWebStorageQuotaErrorAbort = 17,
};

static_assert(static_cast<int>(kWebStorageQuotaErrorNotSupported) ==
                  static_cast<int>(kQuotaErrorNotSupported),
              "WebStorageQuotaError enums do not match");
static_assert(static_cast<int>(kWebStorageQuotaErrorInvalidModification) ==
                  static_cast<int>(kQuotaErrorInvalidModification),
              "WebStorageQuotaError enums do not match");
static_assert(static_cast<int>(kWebStorageQuotaErrorInvalidAccess) ==
                  static_cast<int>(kQuotaErrorInvalidAccess),
              "WebStorageQuotaError enums do not match");
static_assert(static_cast<int>(kWebStorageQuotaErrorAbort) ==
                  static_cast<int>(kQuotaErrorAbort),
              "WebStorageQuotaError enums do not match");

}  // namespace blink

#endif  // WebStorageQuotaError_h
