// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webauth/attestation_statement.h"

namespace content {

AttestationStatement::AttestationStatement(std::string format)
    : format_(std::move(format)) {}

std::string AttestationStatement::GetFormatName() {
  return format_;
}

AttestationStatement::~AttestationStatement() {}

}  // namespace content