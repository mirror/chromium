// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/sys_internals/sys_internals_message_handler.h"

#include <inttypes.h>
#include <cstdio>
#include <sstream>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/sys_info.h"

namespace {
struct CpuInfo {
  int kernel;
  int user;
  int idle;
  int total;
};

void SetConstValue(base::DictionaryValue* result) {
  DCHECK(result);

  result->SetInteger("const.counterMax",
                     SysInternalsMessageHandler::COUNTER_MAX);
}

void CpuInfoInit(std::vector<CpuInfo>* info) {
  DCHECK(info);

  info->clear();
  int num_of_processors = base::SysInfo::NumberOfProcessors();
  for (int i = 0; i < num_of_processors; ++i) {
    info->push_back(CpuInfo());
  }
}

// Get the last 31 bits of the counters.
template <typename T>
inline int ToCounter(T value) {
  return static_cast<int>(value & SysInternalsMessageHandler::COUNTER_MAX);
}

bool ParseProcStatLine(std::vector<CpuInfo>* info, const std::string& line) {
  DCHECK(info);

  uint64_t user = 0;
  uint64_t nice = 0;
  uint64_t sys = 0;
  uint64_t idle = 0;
  uint32_t cpu_index = 0;
  int vals =
      sscanf(line.c_str(),
             "cpu%" PRIu32 " %" PRIu64 " %" PRIu64 " %" PRIu64 " %" PRIu64,
             &cpu_index, &user, &nice, &sys, &idle);
  if (vals != 5 || cpu_index >= info->size()) {
    NOTREACHED();
    return false;
  }

  CpuInfo& cpu_info = (*info)[cpu_index];
  cpu_info.kernel = ToCounter(sys);
  cpu_info.user = ToCounter(user + nice);
  cpu_info.idle = ToCounter(idle);
  cpu_info.total = ToCounter(sys + user + nice + idle);

  return true;
}

const char kProcStat[] = "/proc/stat";

bool GetCpuInfo(std::vector<CpuInfo>* info) {
  DCHECK(info);

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
    if (!ParseProcStatLine(info, line))
      return false;
  }

  return true;
}

void SetCpusValue(base::DictionaryValue* result,
                  const std::vector<CpuInfo>& info) {
  DCHECK(result);

  base::ListValue* cpu_results =
      result->SetList("cpus", base::MakeUnique<base::ListValue>());
  int num_of_processors = static_cast<int>(info.size());
  for (int i = 0; i < num_of_processors; ++i) {
    base::DictionaryValue cpu_result;
    cpu_result.SetInteger("user", info[i].user);
    cpu_result.SetInteger("kernel", info[i].kernel);
    cpu_result.SetInteger("idle", info[i].idle);
    cpu_result.SetInteger("total", info[i].total);
    cpu_results->GetList().push_back(std::move(cpu_result));
  }
}

double GetAvailablePhysicalMemoryKB(const base::SystemMemoryInfoKB& info) {
  double available = 0;
  if (info.available == 0) {
    available = static_cast<double>(info.free + info.reclaimable);
  } else {
    available = static_cast<double>(info.available);
  }
  return available * 1024;
}

void SetMemValue(base::DictionaryValue* result,
                 const base::SystemMemoryInfoKB& info) {
  DCHECK(result);

  // For values that may greater than the range of 32 bit integer, use double.
  result->SetDouble("memory.total", static_cast<double>(info.total) * 1024);
  result->SetDouble("memory.available", GetAvailablePhysicalMemoryKB(info));
  result->SetDouble("memory.swapTotal",
                    static_cast<double>(info.swap_total) * 1024);
  result->SetDouble("memory.swapFree",
                    static_cast<double>(info.swap_free) * 1024);

  result->SetInteger("memory.pswpin", ToCounter(info.pswpin));
  result->SetInteger("memory.pswpout", ToCounter(info.pswpout));
}

void SetZramValue(base::DictionaryValue* result, const base::SwapInfo& info) {
  result->SetInteger("zram.numReads", ToCounter(info.num_reads));
  result->SetInteger("zram.numWrites", ToCounter(info.num_writes));

  // For values that may greater than the range of 32 bit integer, use double.
  result->SetDouble("zram.comprDataSize",
                    static_cast<double>(info.compr_data_size));
  result->SetDouble("zram.origDataSize",
                    static_cast<double>(info.orig_data_size));
  result->SetDouble("zram.memUsedTotal",
                    static_cast<double>(info.mem_used_total));
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
  DCHECK(message);

  if (!IsJavascriptAllowed())
    return;
  CallJavascriptFunction("console.warn", base::Value(message));
}

void SysInternalsMessageHandler::SetValueAndReplySysInfo(
    std::unique_ptr<base::Value> callback_id) {
  std::vector<CpuInfo> cpu_info;
  CpuInfoInit(&cpu_info);
  if (!GetCpuInfo(&cpu_info)) {
    ConsoleWarn("Failed to get system CPU info.");
    cpu_info.clear();
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
  SetCpusValue(&result, cpu_info);
  SetMemValue(&result, mem_info);
  SetZramValue(&result, swap_info);

  ResolveJavascriptCallback(*callback_id, result);
}

void SysInternalsMessageHandler::HandleGetSysInfo(const base::ListValue* args) {
  AllowJavascript();
  DCHECK(args);

  base::Value::ListStorage list = args->GetList();
  if (list.empty())
    return;
  std::unique_ptr<base::Value> callback_id =
      base::MakeUnique<base::Value>(list[0]);

  io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&SysInternalsMessageHandler::SetValueAndReplySysInfo,
                     base::Unretained(this), std::move(callback_id)));
}
