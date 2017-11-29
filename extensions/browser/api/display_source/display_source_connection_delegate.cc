// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/display_source/display_source_connection_delegate.h"

namespace extensions {

DisplaySourceConnectionDelegate::Connection::Connection() = default;

DisplaySourceConnectionDelegate::Connection::~Connection() = default;

DisplaySourceConnectionDelegate::DisplaySourceConnectionDelegate() = default;

DisplaySourceConnectionDelegate::~DisplaySourceConnectionDelegate() = default;

void DisplaySourceConnectionDelegate::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void DisplaySourceConnectionDelegate::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

}  // namespace extensions
