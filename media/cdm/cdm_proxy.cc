// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/cdm/cdm_proxy.h"

namespace media {

KeyInfo::KeyInfo() {}
KeyInfo::~KeyInfo() {}

CdmProxy::CdmProxy(std::unique_ptr<Client> client)
    : client_(std::move(client)) {}
CdmProxy::~CdmProxy() {}

}  // namespace media