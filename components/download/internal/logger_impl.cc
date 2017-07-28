// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/internal/logger_impl.h"

#include "base/memory/ptr_util.h"
#include "base/values.h"

namespace download {

LoggerImpl::LoggerImpl() = default;

LoggerImpl::~LoggerImpl() = default;

std::unique_ptr<base::Value> LoggerImpl::GetServiceStatus() {
  auto result = base::MakeUnique<base::DictionaryValue>();
  result->SetString("serviceState", "READY");
  result->SetString("modelStatus", "OK");
  result->SetString("driverStatus", "OK");
  result->SetString("fileMonitorStatus", "OK");
  return std::move(result);
}

}  // namespace download
