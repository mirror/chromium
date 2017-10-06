// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/dbus/dbus_method_call_status.h"

#include <tuple>

#include "base/bind.h"
#include "base/optional.h"

namespace chromeos {

DBusMethodCallback<std::tuple<>> EmptyVoidDBusMethodCallback() {
  return base::BindOnce([](base::Optional<std::tuple<>> result) {
    // Do nothing.
  });
}

}  // namespace chromeos
