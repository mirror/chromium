// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/networking_onc/networking_onc_delegate.h"

#include "extensions/browser/api/networking_onc/networking_onc_api.h"

namespace extensions {

NetworkingOncDelegate::NetworkingOncDelegate() {}

NetworkingOncDelegate::~NetworkingOncDelegate() {}

void NetworkingOncDelegate::AddObserver(
    NetworkingOncDelegateObserver* observer) {
  NOTREACHED() << "Class does not use NetworkingOncDelegateObserver";
}

void NetworkingOncDelegate::RemoveObserver(
    NetworkingOncDelegateObserver* observer) {
  NOTREACHED() << "Class does not use NetworkingOncDelegateObserver";
}

void NetworkingOncDelegate::StartActivate(
    const std::string& guid,
    const std::string& carrier,
    const VoidCallback& success_callback,
    const FailureCallback& failure_callback) {
  failure_callback.Run(networking_onc::kErrorNotSupported);
}

}  // namespace extensions
