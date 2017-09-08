// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/process_metrics.h"

#include <utility>

#include "base/process/process_metrics.h"
#include "base/time/time.h"
#include "build/build_config.h"

namespace remoting {

namespace {

// TODO(zijiehe): Add PortProvider parameter for Mac OSX.
std::unique_ptr<base::ProcessMetrics> CreateProcessMetrics(
    base::ProcessHandle handle) {
#if !defined(OS_MACOSX) || defined(OS_IOS)
  return base::ProcessMetrics::CreateProcessMetrics(handle);
#else
  return base::ProcessMetrics::CreateProcessMetrics(handle, nullptr);
#endif
}

}  // namespace

ProcessMetrics::ProcessMetrics(const std::string& name, base::Process&& process)
    : name_(name),
      process_(std::move(process)),
      metrics_(CreateProcessMetrics(process_.Handle())) {}

ProcessMetrics::~ProcessMetrics() = default;

bool ProcessMetrics::IsValid() const {
  return process_.IsValid() && metrics_;
}

bool ProcessMetrics::IsRunning() const {
  return !process_.WaitForExitWithTimeout(base::TimeDelta(), nullptr);
}

protocol::ProcessResourceUsage ProcessMetrics::GetResourceUsage() {
  protocol::ProcessResourceUsage result;
  if (IsValid() && IsRunning()) {
    result.set_process_name(name_);
    result.set_processor_usage(metrics_->GetPlatformIndependentCPUUsage());
    result.set_working_set_size(metrics_->GetWorkingSetSize());
    result.set_pagefile_size(metrics_->GetPagefileUsage());
  }
  return result;
}

bool ProcessMetrics::TerminateForTesting() {
  return process_.Terminate(0, true);
}

}  // namespace remoting
