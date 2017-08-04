// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/memory/near_oom_detector_android.h"

#include "base/command_line.h"
#include "base/process/process_metrics.h"
#include "base/strings/string_number_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/browser/memory/memory_monitor_android.h"
#include "content/public/browser/render_process_host.h"

namespace content {

namespace {

constexpr base::TimeDelta kDefaultMonitoringTimeDelta =
    base::TimeDelta::FromSeconds(1);
constexpr base::TimeDelta kDefaultCooldownTimeDelta =
    base::TimeDelta::FromSeconds(60);
const int64_t kMarginFactor = 3;

}  // namespace

NearOOMDetector* NearOOMDetector::GetInstance() {
  static NearOOMDetector* instance = new NearOOMDetector();
  return instance;
}

NearOOMDetector::NearOOMDetector()
    : task_runner_(base::ThreadTaskRunnerHandle::Get()),
      memory_monitor_(MemoryMonitorAndroid::Create()),
      monitoring_interval_(kDefaultMonitoringTimeDelta),
      cooldown_interval_(kDefaultCooldownTimeDelta) {}

NearOOMDetector::~NearOOMDetector() = default;

bool NearOOMDetector::Start() {
  MemoryMonitorAndroid::MemoryInfo memory_info;
  memory_monitor_->GetMemoryInfo(&memory_info);
  if (memory_info.foreground_app_threshold == 0)
    return false;

  const std::string& value =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          "near-oom-threshold-bytes");
  if (!value.empty()) {
    base::StringToInt64(value, &near_oom_threshold_);
  } else {
    int64_t margin = memory_info.foreground_app_threshold / kMarginFactor;
    near_oom_threshold_ = memory_info.foreground_app_threshold + margin;
  }

  ScheduleCheck(monitoring_interval_);
  return true;
}

void NearOOMDetector::ScheduleCheck(base::TimeDelta delay) {
  check_closure_.Reset(
      base::Bind(&NearOOMDetector::Check, base::Unretained(this)));
  task_runner_->PostDelayedTask(FROM_HERE, check_closure_.callback(), delay);
}

void NearOOMDetector::Check() {
  base::SystemMemoryInfoKB system_memory_info;
  bool ok = GetSystemMemoryInfo(&system_memory_info);
  CHECK(ok);

  int64_t cached_bytes = system_memory_info.cached * 1024;
  OutOfMemoryCondition condition = CalculateCondition(cached_bytes);
  if (condition != last_condition_) {
    for (RenderProcessHost::iterator it(RenderProcessHost::AllHostsIterator());
         !it.IsAtEnd(); it.Advance()) {
      it.GetCurrentValue()
          ->GetRendererInterface()
          ->OnOutOfMemoryConditionChanged(condition);
    }
    last_condition_ = condition;
  }

  if (condition == OutOfMemoryCondition::NEAR_OOM) {
    ScheduleCheck(cooldown_interval_);
  } else {
    ScheduleCheck(monitoring_interval_);
  }
}

OutOfMemoryCondition NearOOMDetector::CalculateCondition(int64_t cached) {
  if (cached < near_oom_threshold_)
    return OutOfMemoryCondition::NEAR_OOM;
  return OutOfMemoryCondition::NORMAL;
}

}  // namespace content
