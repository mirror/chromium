// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/tools/cert_mapper/entry.h"

namespace net {

CertBytes::CertBytes(const void* data, size_t length)
    : bytes_(static_cast<const uint8_t*>(data),
             static_cast<const uint8_t*>(data) + length) {}

std::string CertBytes::AsString() const {
  return std::string(bytes_.begin(), bytes_.end());
}

base::StringPiece CertBytes::AsStringPiece() const {
  return base::StringPiece(reinterpret_cast<const char*>(bytes_.data()),
                           bytes_.size());
}

der::Input CertBytes::AsInput() const {
  return der::Input(bytes_.data(), bytes_.size());
}

CertBytes::~CertBytes() = default;

Entry::Entry() {}

Entry::Entry(const Entry&) = default;
Entry::Entry(Entry&&) = default;

Entry::~Entry() = default;

}  // namespace net
