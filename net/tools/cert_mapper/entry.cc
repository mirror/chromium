// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/tools/cert_mapper/entry.h"

namespace net {

Entry::Entry() {}

Entry::Entry(const der::Input& cert) {}

Entry::Entry(Type type, const der::Input& cert) : type(type) {
  certs.push_back(cert);
}

Entry::Entry(const Entry&) = default;

Entry::~Entry() {}

}  // namespace net
