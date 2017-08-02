// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Enums that specify values that specify the results of download checking as
// performed by SafeBrowsing download protection service.

#ifndef CHROME_BROWSER_SAFE_BROWSING_DOWNLOAD_PROTECTION_DOWNLOAD_CHECK_ENUMS_H_
#define CHROME_BROWSER_SAFE_BROWSING_DOWNLOAD_PROTECTION_DOWNLOAD_CHECK_ENUMS_H_

#include "base/callback_list.h"
#include "components/safe_browsing/csd.pb.h"
#include "content/public/browser/download_item.h"

namespace safe_browsing {

// TODO this is copied from download_protection_service
const void* const kDownloadReferrerChainDataKey =
    &kDownloadReferrerChainDataKey;

enum DownloadCheckResult {
  UNKNOWN,
  SAFE,
  DANGEROUS,
  UNCOMMON,
  DANGEROUS_HOST,
  POTENTIALLY_UNWANTED
};

// Enum to keep track why a particular download verdict was chosen.
// Used for UMA metrics. Do not reorder.
enum DownloadCheckResultReason {
  REASON_INVALID_URL = 0,
  REASON_SB_DISABLED = 1,
  REASON_WHITELISTED_URL = 2,
  REASON_WHITELISTED_REFERRER = 3,
  REASON_INVALID_REQUEST_PROTO = 4,
  REASON_SERVER_PING_FAILED = 5,
  REASON_INVALID_RESPONSE_PROTO = 6,
  REASON_NOT_BINARY_FILE = 7,
  REASON_REQUEST_CANCELED = 8,
  REASON_DOWNLOAD_DANGEROUS = 9,
  REASON_DOWNLOAD_SAFE = 10,
  REASON_EMPTY_URL_CHAIN = 11,
  DEPRECATED_REASON_HTTPS_URL = 12,
  REASON_PING_DISABLED = 13,
  REASON_TRUSTED_EXECUTABLE = 14,
  REASON_OS_NOT_SUPPORTED = 15,
  REASON_DOWNLOAD_UNCOMMON = 16,
  REASON_DOWNLOAD_NOT_SUPPORTED = 17,
  REASON_INVALID_RESPONSE_VERDICT = 18,
  REASON_ARCHIVE_WITHOUT_BINARIES = 19,
  REASON_DOWNLOAD_DANGEROUS_HOST = 20,
  REASON_DOWNLOAD_POTENTIALLY_UNWANTED = 21,
  REASON_UNSUPPORTED_URL_SCHEME = 22,
  REASON_MANUAL_BLACKLIST = 23,
  REASON_LOCAL_FILE = 24,
  REASON_REMOTE_FILE = 25,
  REASON_SAMPLED_UNSUPPORTED_FILE = 26,
  REASON_VERDICT_UNKNOWN = 27,
  REASON_MAX  // Always add new values before this one.
};

// Enumerate for histogramming purposes.
// DO NOT CHANGE THE ORDERING OF THESE VALUES (different histogram data will
// be mixed together based on their values).
enum SBStatsType {
  DOWNLOAD_URL_CHECKS_TOTAL,
  DOWNLOAD_URL_CHECKS_CANCELED,
  DOWNLOAD_URL_CHECKS_MALWARE,

  DOWNLOAD_HASH_CHECKS_TOTAL,
  DOWNLOAD_HASH_CHECKS_MALWARE,

  // Memory space for histograms is determined by the max.
  // ALWAYS ADD NEW VALUES BEFORE THIS ONE.
  DOWNLOAD_CHECKS_MAX
};

enum WhitelistType {
  NO_WHITELIST_MATCH,
  URL_WHITELIST,
  SIGNATURE_WHITELIST,
  WHITELIST_TYPE_MAX
};

}  // namespace safe_browsing

#endif  // CHROME_BROWSER_SAFE_BROWSING_DOWNLOAD_PROTECTION_DOWNLOAD_CHECK_ENUMS_H_
