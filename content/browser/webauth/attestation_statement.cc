// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webauth/attestation_statement.h"

namespace content {
namespace authenticator_utils {

AttestationStatement::AttestationStatement(const std::string& format)
    : format_(format) {}

std::string AttestationStatement::GetFormat() {
  return format_;
}

AttestationStatement::~AttestationStatement() {}

}  // namespace authenticator_utils
}  // namespace content