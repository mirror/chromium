// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/sys_internals/sys_internals_message_handler.h"

#include <inttypes.h>
#include <cstdio>
#include <sstream>
#include <string>
#include <utility>
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

void SetConstValue(base::Value* result) {
  DCHECK(result);

  int counter_max = static_cast<int>(SysInternalsMessageHandler::COUNTER_MAX);
  result->SetPath({"const", "counterMax"}, base::Value(counter_max));
}

void CpuInfoInit(std::vector<CpuInfo>* info) {
  DCHECK(info);

  info->clear();
  int num_of_processors = base::SysInfo::NumberOfProcessors();
  for (int i = 0; i < num_of_processors; ++i) {
    info->push_back(CpuInfo());
  }
}

// Gets the last 31 bits of the counter value.
template <typename T>
inline int ToCounter(T value) {
  return static_cast<int>(value & SysInternalsMessageHandler::COUNTER_MAX);
}

bool ParseProcStatLine(const std::string& line, std::vector<CpuInfo>* info) {
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
    if (!ParseProcStatLine(line, info))
      return false;
  }

  return true;
}

void SetCpusValue(const std::vector<CpuInfo>& info, base::Value* result) {
  DCHECK(result);

  base::Value cpu_results(base::Value::Type::LIST);
  for (const CpuInfo& cpu : info) {
    base::Value cpu_result(base::Value::Type::DICTIONARY);
    cpu_result.SetKey("user", base::Value(cpu.user));
    cpu_result.SetKey("kernel", base::Value(cpu.kernel));
    cpu_result.SetKey("idle", base::Value(cpu.idle));
    cpu_result.SetKey("total", base::Value(cpu.total));
    cpu_results.GetList().push_back(std::move(cpu_result));
  }
  result->SetKey("cpus", std::move(cpu_results));
}

double GetAvailablePhysicalMemory(const base::SystemMemoryInfoKB& info) {
  double available = 0;
  if (info.available == 0) {
    available = static_cast<double>(info.free + info.reclaimable);
  } else {
    available = static_cast<double>(info.available);
  }
  return available * 1024;
}

void SetMemValue(const base::SystemMemoryInfoKB& info, base::Value* result) {
  DCHECK(result);

  base::Value mem_result(base::Value::Type::DICTIONARY);

  // For values that may exceed the range of 32-bit signed integer, use double.
  double total = static_cast<double>(info.total) * 1024;
  mem_result.SetKey("total", base::Value(total));
  mem_result.SetKey("available", base::Value(GetAvailablePhysicalMemory(info)));
  double swap_total = static_cast<double>(info.swap_total) * 1024;
  mem_result.SetKey("swapTotal", base::Value(swap_total));
  double swap_free = static_cast<double>(info.swap_free) * 1024;
  mem_result.SetKey("swapFree", base::Value(swap_free));

  mem_result.SetKey("pswpin", base::Value(ToCounter(info.pswpin)));
  mem_result.SetKey("pswpout", base::Value(ToCounter(info.pswpout)));

  result->SetKey("memory", std::move(mem_result));
}

void SetZramValue(const base::SwapInfo& info, base::Value* result) {
  DCHECK(result);
  base::Value zram_result(base::Value::Type::DICTIONARY);

  zram_result.SetKey("numReads", base::Value(ToCounter(info.num_reads)));
  zram_result.SetKey("numWrites", base::Value(ToCounter(info.num_writes)));

  // For values that may exceed the range of 32-bit signed integer, use double.
  zram_result.SetKey("comprDataSize",
                     base::Value(static_cast<double>(info.compr_data_size)));
  zram_result.SetKey("origDataSize",
                     base::Value(static_cast<double>(info.orig_data_size)));
  zram_result.SetKey("memUsedTotal",
                     base::Value(static_cast<double>(info.mem_used_total)));

  result->SetKey("zram", std::move(zram_result));
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

  base::Value result(base::Value::Type::DICTIONARY);
  SetConstValue(&result);
  SetCpusValue(cpu_info, &result);
  SetMemValue(mem_info, &result);
  SetZramValue(swap_info, &result);

  ResolveJavascriptCallback(*callback_id, result);
}

void SysInternalsMessageHandler::HandleGetSysInfo(const base::ListValue* args) {
  AllowJavascript();
  DCHECK(args);

  const base::Value::ListStorage& list = args->GetList();
  if (list.empty())
    return;
  std::unique_ptr<base::Value> callback_id =
      base::MakeUnique<base::Value>(list[0].Clone());

  io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&SysInternalsMessageHandler::SetValueAndReplySysInfo,
                     base::Unretained(this), std::move(callback_id)));
}
