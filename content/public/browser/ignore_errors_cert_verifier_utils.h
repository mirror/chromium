// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_IGNORE_ERRORS_CERT_VERIFIER_UTILS_H_
#define CONTENT_PUBLIC_BROWSER_IGNORE_ERRORS_CERT_VERIFIER_UTILS_H_

#include <memory>

#include "base/command_line.h"
#include "content/common/content_export.h"
#include "net/cert/cert_verifier.h"

namespace content {

// MaybeWrapCertVerifier returns an IgnoreErrorsCertVerifier wrapping the
// supplied verifier using the whitelist from the
// --ignore-certificate-errors-spki-list flag of the command line if the
// --user-data-dir flag is also present. If either of these flags are missing,
// it returns the supplied verifier.
// As the --user-data-dir flag is embedder defined, the flag to check for
// needs to be passed in.
CONTENT_EXPORT std::unique_ptr<net::CertVerifier> MaybeWrapCertVerifier(
    const base::CommandLine& command_line,
    const char* user_data_dir_switch,
    std::unique_ptr<net::CertVerifier> verifier);

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_IGNORE_ERRORS_CERT_VERIFIER_UTILS_H_