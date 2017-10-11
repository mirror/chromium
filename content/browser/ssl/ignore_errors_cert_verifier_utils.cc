// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/ignore_errors_cert_verifier_utils.h"

#include "base/memory/ptr_util.h"
#include "base/strings/string_split.h"
#include "content/public/common/content_switches.h"
#include "net/cert/ignore_errors_cert_verifier.h"

using ::net::CertVerifier;
using ::net::IgnoreErrorsCertVerifier;

namespace content {

std::unique_ptr<CertVerifier> MaybeWrapCertVerifier(
    const base::CommandLine& command_line,
    const char* user_data_dir_switch,
    std::unique_ptr<CertVerifier> verifier) {
  if (!command_line.HasSwitch(user_data_dir_switch) ||
      !command_line.HasSwitch(switches::kIgnoreCertificateErrorsSPKIList)) {
    return verifier;
  }
  auto spki_list =
      base::SplitString(command_line.GetSwitchValueASCII(
                            switches::kIgnoreCertificateErrorsSPKIList),
                        ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  return base::MakeUnique<IgnoreErrorsCertVerifier>(
      std::move(verifier), IgnoreErrorsCertVerifier::MakeWhitelist(spki_list));
}

}  // namespace content
