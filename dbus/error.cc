// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "dbus/error.h"

namespace dbus {

Error::Error(const std::string& name, const std::string& message)
    : name_(name), message_(message) {}

Error::~Error() = default;

}  // namespace dbus
