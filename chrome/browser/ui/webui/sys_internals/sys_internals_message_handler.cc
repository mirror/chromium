// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/sys_internals/sys_internals_message_handler.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/memory/ptr_util.h"
#include "base/process/process_metrics.h"
#include "base/sys_info.h"
#include "base/task_scheduler/post_task.h"
#include "base/values.h"
#include "content/public/browser/web_ui_message_handler.h"

namespace {
void SetConstValue(base::DictionaryValue* result) {
  result->SetInteger("const.counterMax",
                     SysInternalsMessageHandler::COUNTER_MAX);
}

#if defined(OS_WIN) || defined(OS_MACOSX) || defined(OS_LINUX)
void CpuInfoInit(std::vector<base::CpuInfo>* infos) {
  infos->clear();
  int num_of_processors = base::SysInfo::NumberOfProcessors();
  for (int i = 0; i < num_of_processors; ++i) {
    infos->push_back(base::CpuInfo());
  }
}
#endif

double GetAvailablePhysicalMemoryKB(const base::SystemMemoryInfoKB* info) {
  double available = 0;
#if defined(OS_WIN)
  available = static_cast<double>(info->avail_phys);
#elif defined(OS_MACOSX) || defined(OS_IOS)
  available = static_cast<double>(info->free + info->speculative);
#elif defined(OS_LINUX) || defined(OS_ANDROID)
  if (info->available == 0) {
    available = static_cast<double>(info->free + info->reclaimable);
  } else {
    available = static_cast<double>(info->available);
  }
#endif  // TODO: add other platforms.
  return available * 1024;
}

void SetCpusValue(base::DictionaryValue* result,
                  const std::vector<base::CpuInfo>* infos) {
  base::ListValue cpu_results;
  int num_of_processors = static_cast<int>(infos->size());
  for (int i = 0; i < num_of_processors; ++i) {
    base::DictionaryValue cpu_result;
    cpu_result.SetInteger("user", infos->at(i).user);
    cpu_result.SetInteger("kernel", infos->at(i).kernel);
    cpu_result.SetInteger("idle", infos->at(i).idle);
    cpu_result.SetInteger("total", infos->at(i).total);
    cpu_results.GetList().push_back(std::move(cpu_result));
  }
  result->SetList("cpus",
                  base::MakeUnique<base::ListValue>(std::move(cpu_results)));
}

void SetMemValue(base::DictionaryValue* result,
                 const base::SystemMemoryInfoKB* info) {
  result->SetDouble("memory.total", static_cast<double>(info->total) * 1024);
  result->SetDouble("memory.available", GetAvailablePhysicalMemoryKB(info));
#if !defined(OS_MACOSX)
  result->SetDouble("memory.swapTotal",
                    static_cast<double>(info->swap_total) * 1024);
  result->SetDouble("memory.swapFree",
                    static_cast<double>(info->swap_free) * 1024);
#endif

#if defined(OS_ANDROID) || defined(OS_LINUX)
  result->SetInteger(
      "memory.pswpin",
      static_cast<int>(info->pswpin & SysInternalsMessageHandler::COUNTER_MAX));
  result->SetInteger("memory.pswpout",
                     static_cast<int>(info->pswpout &
                                      SysInternalsMessageHandler::COUNTER_MAX));
#endif
}

#if defined(OS_CHROMEOS)
void SetZramValue(base::DictionaryValue* result, const base::SwapInfo* info) {
  result->SetInteger("zram.numReads",
                     static_cast<int>(info->num_reads &
                                      SysInternalsMessageHandler::COUNTER_MAX));
  result->SetInteger("zram.numWrites",
                     static_cast<int>(info->num_writes &
                                      SysInternalsMessageHandler::COUNTER_MAX));
  result->SetDouble("zram.comprDataSize",
                    static_cast<double>(info->compr_data_size));
  result->SetDouble("zram.origDataSize",
                    static_cast<double>(info->orig_data_size));
  result->SetDouble("zram.memUsedTotal",
                    static_cast<double>(info->mem_used_total));
}
#endif

}  // namespace

SysInternalsMessageHandler::SysInternalsMessageHandler() {}

SysInternalsMessageHandler::~SysInternalsMessageHandler() {}

void SysInternalsMessageHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "getSysInfo", base::Bind(&SysInternalsMessageHandler::GetSysInfo,
                               base::Unretained(this)));
}

void SysInternalsMessageHandler::GetSysInfo(const base::ListValue* args) {
  AllowJavascript();

  scoped_refptr<base::SequencedTaskRunner> sequenced_task_runner =
      base::CreateSequencedTaskRunnerWithTraits(base::MayBlock());

  sequenced_task_runner->PostTask(
      FROM_HERE,
      base::BindOnce(&SysInternalsMessageHandler::UpdateInfoStructures,
                     base::Unretained(this)));

  std::unique_ptr<base::Value> time_tick;
  bool is_console_log = true;
  if (args->GetList().size() > 0) {
    time_tick = base::MakeUnique<base::Value>(args->GetList()[0]);
    is_console_log = false;
  }
  sequenced_task_runner->PostTask(
      FROM_HERE,
      base::BindOnce(&SysInternalsMessageHandler::SetValueAndReplySysInfo,
                     base::Unretained(this), std::move(time_tick),
                     is_console_log));
}

void SysInternalsMessageHandler::SetValueAndReplySysInfo(
    std::unique_ptr<base::Value> time_tick,
    bool is_console_log) {
  base::DictionaryValue result;
  SetConstValue(&result);
#if defined(OS_WIN) || defined(OS_MACOSX) || defined(OS_LINUX)
  SetCpusValue(&result, &cpu_infos_);
#endif
  SetMemValue(&result, &mem_info_);
#if defined(OS_CHROMEOS)
  SetZramValue(&result, &swap_info_);
#endif  // defined(OS_CHROMEOS)
  if (is_console_log) {
    CallJavascriptFunction("console.log", result);
  } else {
    result.Set("timeTick", std::move(time_tick));
    CallJavascriptFunction("SysInternals.handleUpdateDatas", result);
  }
}

void SysInternalsMessageHandler::ConsoleWarn(const char* message) {
  if (!IsJavascriptAllowed())
    return;
  CallJavascriptFunction("console.warn", base::Value(message));
}

void SysInternalsMessageHandler::UpdateInfoStructures() {
#if defined(OS_WIN) || defined(OS_MACOSX) || defined(OS_LINUX)
  CpuInfoInit(&cpu_infos_);
  if (!GetCpuInfo(&cpu_infos_)) {
    ConsoleWarn("Failed to get system CPU info.");
    cpu_infos_.clear();
  }
#endif
  if (!GetSystemMemoryInfo(&mem_info_)) {
    ConsoleWarn("Failed to get system memory info.");
  }
#if defined(OS_CHROMEOS)
  if (!GetSwapInfo(&swap_info_)) {
    ConsoleWarn("Failed to get system zram info.");
  }
#endif  // defined(OS_CHROMEOS)
}