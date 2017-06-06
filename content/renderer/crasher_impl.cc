// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/crasher_impl.h"

#include <utility>

#include "base/debug/crash_logging.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"

namespace content {

void CrasherImpl::Create(const service_manager::BindSourceInfo&,
                         mojom::CrasherRequest request) {
  new CrasherImpl(std::move(request));
}

CrasherImpl::CrasherImpl(mojom::CrasherRequest request)
    : binding_(this, std::move(request)) {}

CrasherImpl::~CrasherImpl() = default;

void CrasherImpl::Crash(mojom::CrashKeysPtr crash_keys) {
  base::debug::SetCrashKeyValue(
      "free_memory_ratio_in_percent",
      base::IntToString(crash_keys->free_memory_ratio));
  IMMEDIATE_CRASH();
}

}  // namespace content
