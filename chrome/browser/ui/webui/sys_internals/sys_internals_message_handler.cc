// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/sys_internals/sys_internals_message_handler.h"

#include <stdint.h>
#include <cstdio>
#include <sstream>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/files/file_util.h"
#include "base/format_macros.h"
#include "base/memory/ptr_util.h"
#include "base/process/process_metrics.h"
#include "base/sys_info.h"
#include "base/values.h"
#include "content/public/browser/web_ui_message_handler.h"

namespace {
struct CpuInfo {
  int kernel;
  int user;
  int idle;
  int total;
};

void SetConstValue(base::DictionaryValue* result) {
  result->SetInteger("const.counterMax",
                     SysInternalsMessageHandler::COUNTER_MAX);
}

void CpuInfoInit(std::vector<CpuInfo>* infos) {
  infos->clear();
  int num_of_processors = base::SysInfo::NumberOfProcessors();
  for (int i = 0; i < num_of_processors; ++i) {
    infos->push_back(CpuInfo());
  }
}

bool ParseProcStatLine(std::vector<CpuInfo>* infos, std::string* line) {
  uint64_t user = 0;
  uint64_t nice = 0;
  uint64_t sys = 0;
  uint64_t idle = 0;
  uint32_t pindex = 0;
  int vals =
      sscanf(line->c_str(),
             "cpu%" PRIu32 " %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64,
             &pindex, &user, &nice, &sys, &idle);
  if (vals != 5 || pindex >= infos->size()) {
    NOTREACHED();
    return false;
  }

  // Get the last 31 bits of the counters.
  uint32_t counter_max = SysInternalsMessageHandler::COUNTER_MAX;
  infos->at(pindex).kernel = static_cast<int>(sys & counter_max);
  infos->at(pindex).user = static_cast<int>((user + nice) & counter_max);
  infos->at(pindex).idle = static_cast<int>(idle & counter_max);
  infos->at(pindex).total =
      static_cast<int>((sys + user + nice + idle) & counter_max);

  return true;
}

const char kProcStat[] = "/proc/stat";

bool GetCpuInfo(std::vector<CpuInfo>* infos) {
  DCHECK(infos);

  // WARNING: this method may return incomplete data because some processors may
  // be brought offline at runtime. /proc/stat does not report statistics of
  // offline processors. CPU usages of offline processors will be filled with
  // zeros.
  //
  // An example of output of /proc/stat when processor 0 and 3 are online, but
  // processor 1 and 2 are offline:
  //
  //   cpu  145292 20018 83444 1485410 995 44 3578 0 0 0
  //   cpu0 138060 19947 78350 1479514 570 44 3576 0 0 0
  //   cpu3 2033 32 1075 1400 52 0 1 0 0 0
  std::string contents;
  if (!base::ReadFileToString(base::FilePath(kProcStat), &contents))
    return false;

  std::istringstream iss(contents);
  std::string line;

  // Skip the first line because it is just an aggregated number of
  // all cpuN lines.
  std::getline(iss, line);
  while (std::getline(iss, line)) {
    if (line.compare(0, 3, "cpu") != 0)
      continue;
    if (!ParseProcStatLine(infos, &line))
      return false;
  }

  return true;
}

void SetCpusValue(base::DictionaryValue* result,
                  const std::vector<CpuInfo>* infos) {
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

double GetAvailablePhysicalMemoryKB(const base::SystemMemoryInfoKB* info) {
  double available = 0;
  if (info->available == 0) {
    available = static_cast<double>(info->free + info->reclaimable);
  } else {
    available = static_cast<double>(info->available);
  }
  return available * 1024;
}

void SetMemValue(base::DictionaryValue* result,
                 const base::SystemMemoryInfoKB* info) {
  // For values that may greater than the range of 32 bit integer, use double.
  result->SetDouble("memory.total", static_cast<double>(info->total) * 1024);
  result->SetDouble("memory.available", GetAvailablePhysicalMemoryKB(info));
  result->SetDouble("memory.swapTotal",
                    static_cast<double>(info->swap_total) * 1024);
  result->SetDouble("memory.swapFree",
                    static_cast<double>(info->swap_free) * 1024);

  // Get the last 31 bits of the counters.
  result->SetInteger(
      "memory.pswpin",
      static_cast<int>(info->pswpin & SysInternalsMessageHandler::COUNTER_MAX));
  result->SetInteger("memory.pswpout",
                     static_cast<int>(info->pswpout &
                                      SysInternalsMessageHandler::COUNTER_MAX));
}

void SetZramValue(base::DictionaryValue* result, const base::SwapInfo* info) {
  // Get the last 31 bits of the counters.
  result->SetInteger("zram.numReads",
                     static_cast<int>(info->num_reads &
                                      SysInternalsMessageHandler::COUNTER_MAX));
  result->SetInteger("zram.numWrites",
                     static_cast<int>(info->num_writes &
                                      SysInternalsMessageHandler::COUNTER_MAX));

  // For values that may greater than the range of 32 bit integer, use double.
  result->SetDouble("zram.comprDataSize",
                    static_cast<double>(info->compr_data_size));
  result->SetDouble("zram.origDataSize",
                    static_cast<double>(info->orig_data_size));
  result->SetDouble("zram.memUsedTotal",
                    static_cast<double>(info->mem_used_total));
}

}  // namespace

SysInternalsMessageHandler::SysInternalsMessageHandler() {
  io_task_runner_ = base::CreateSequencedTaskRunnerWithTraits(base::MayBlock());
}

SysInternalsMessageHandler::~SysInternalsMessageHandler() {}

void SysInternalsMessageHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "getSysInfo", base::Bind(&SysInternalsMessageHandler::HandleGetSysInfo,
                               base::Unretained(this)));
}

void SysInternalsMessageHandler::ConsoleWarn(const char* message) {
  if (!IsJavascriptAllowed())
    return;
  CallJavascriptFunction("console.warn", base::Value(message));
}

void SysInternalsMessageHandler::SetValueAndReplySysInfo(
    std::unique_ptr<base::Value> time_tick,
    bool is_console_log) {
  std::vector<CpuInfo> cpu_infos;
  CpuInfoInit(&cpu_infos);
  if (!GetCpuInfo(&cpu_infos)) {
    ConsoleWarn("Failed to get system CPU info.");
    cpu_infos.clear();
  }
  base::SystemMemoryInfoKB mem_info;
  if (!GetSystemMemoryInfo(&mem_info)) {
    ConsoleWarn("Failed to get system memory info.");
  }
  base::SwapInfo swap_info;
  if (!GetSwapInfo(&swap_info)) {
    ConsoleWarn("Failed to get system zram info.");
  }

  base::DictionaryValue result;
  SetConstValue(&result);
  SetCpusValue(&result, &cpu_infos);
  SetMemValue(&result, &mem_info);
  SetZramValue(&result, &swap_info);

  if (is_console_log) {
    CallJavascriptFunction("console.log", result);
  } else {
    result.Set("timeTick", std::move(time_tick));
    CallJavascriptFunction("SysInternals.handleUpdateData", result);
  }
}

void SysInternalsMessageHandler::HandleGetSysInfo(const base::ListValue* args) {
  AllowJavascript();

  std::unique_ptr<base::Value> time_tick;
  bool is_console_log = true;
  if (args->GetList().size() > 0) {
    time_tick = base::MakeUnique<base::Value>(args->GetList()[0]);
    is_console_log = false;
  }
  io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&SysInternalsMessageHandler::SetValueAndReplySysInfo,
                     base::Unretained(this), std::move(time_tick),
                     is_console_log));
}
