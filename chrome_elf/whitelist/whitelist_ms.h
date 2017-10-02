// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_ELF_WHITELIST_WHITELIST_MS_H_
#define CHROME_ELF_WHITELIST_WHITELIST_MS_H_

namespace whitelist {

enum MsStatus {
  MS_SUCCESS = 0,
  MS_BLOCK_BINARY,
  MS_FILE_HASH_FAIL,
  MS_GET_ADMIN_CONTEXT_FAIL,
  MS_NO_CATALOG,
  MS_GET_CATALOG_INFO_FROM_CONTEXT_FAIL,
  MS_GET_SUBJECT_FROM_CATALOG_FAIL,
  MS_NO_EMBEDDED_CERT,
  MS_GET_SIGNER_INFO_FAIL,
  MS_GET_CERT_FOR_SIGNER_FAIL,
  MS_GET_SUBJECT_FROM_CERT_FAIL,
  MS_FILE_NOT_SIGNED,
};

// Check PE file for whitelisting.
//
// NOTE:
//   - MS_SUCCESS: whitelist.
//   - MS_BLOCK_BINARY: failed whitelist check.
MsStatus CheckImageSignature(const std::wstring& path);

}  // namespace whitelist

#endif  // CHROME_ELF_WHITELIST_WHITELIST_MS_H_
