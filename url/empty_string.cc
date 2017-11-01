// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "url/empty_string.h"

#include "base/lazy_instance.h"

namespace url {
namespace internal {

namespace {

base::LazyInstance<std::string>::Leaky empty_string = LAZY_INSTANCE_INITIALIZER;

}  // namespace

const std::string& EmptyString() {
  return empty_string.Get();
}

}  // namespace internal
}  // namespace url
