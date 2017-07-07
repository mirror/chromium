// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/utility/content_utility_client.h"

#include "content/common/content_client_shutdown_helper.h"

namespace content {

ContentUtilityClient::~ContentUtilityClient() {
  ContentClientShutdownHelper::ContentClientPartDeleted(this);
}

bool ContentUtilityClient::OnMessageReceived(const IPC::Message& message) {
  return false;
}

}  // namespace content
