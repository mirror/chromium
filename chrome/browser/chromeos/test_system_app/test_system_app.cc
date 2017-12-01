// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/test_system_app/test_system_app.h"

#include <utility>

TestSystemAppImpl::TestSystemAppImpl(
    mojo::InterfaceRequest<mojom::TestSystemApp> request)
    : binding_(this, std::move(request)) {}

TestSystemAppImpl::~TestSystemAppImpl() {}

void TestSystemAppImpl::GetNumber(GetNumberCallback callback) {
  std::move(callback).Run(42);
}

