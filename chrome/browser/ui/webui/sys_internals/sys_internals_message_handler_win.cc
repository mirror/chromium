// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/sys_internals/sys_internals_message_handler.h"

#include <windows.h>
#include <winternl.h>

#include <memory>

#include "base/sys_info.h"

namespace {

const wchar_t kNtdll[] = L"ntdll.dll";
const char kNtQuerySystemInformationName[] = "NtQuerySystemInformation";

// See MSDN about NtQuerySystemInformation definition.
typedef DWORD(WINAPI* NtQuerySystemInformationPF)(DWORD system_info_class,
                                                  PVOID system_info,
                                                  ULONG system_info_length,
                                                  PULONG return_length);

}  // namespace

bool SysInternalsMessageHandler::GetCpuInfo(std::vector<base::CpuInfo>* infos) {
  DCHECK(infos);

  HMODULE ntdll = GetModuleHandle(kNtdll);
  CHECK(ntdll != NULL);
  NtQuerySystemInformationPF NtQuerySystemInformation =
      reinterpret_cast<NtQuerySystemInformationPF>(
          ::GetProcAddress(ntdll, kNtQuerySystemInformationName));

  CHECK(NtQuerySystemInformation != NULL);

  int num_of_processors = base::SysInfo::NumberOfProcessors();
  std::unique_ptr<SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION[]> processor_info(
      new SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION[num_of_processors]);

  ULONG returned_bytes = 0,
        bytes = sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) *
                num_of_processors;
  if (!NT_SUCCESS(NtQuerySystemInformation(
          SystemProcessorPerformanceInformation, processor_info.get(), bytes,
          &returned_bytes)))
    return false;

  int returned_num_of_processors =
      returned_bytes / sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION);

  if (returned_num_of_processors != num_of_processors)
    return false;

  DCHECK_EQ(num_of_processors, static_cast<int>(infos->size()));
  for (int i = 0; i < returned_num_of_processors; ++i) {
    uint64_t kernel =
        static_cast<uint64_t>(processor_info[i].KernelTime.QuadPart);
    uint64_t user = static_cast<uint64_t>(processor_info[i].UserTime.QuadPart);
    uint64_t idle = static_cast<uint64_t>(processor_info[i].IdleTime.QuadPart);

    uint32_t counter_max = SysInternalsMessageHandler::COUNTER_MAX;
    // KernelTime needs to be fixed-up, because it includes both idle time and
    // real kernel time.
    infos->at(i).kernel = static_cast<int>((kernel - idle) & counter_max);
    infos->at(i).user = static_cast<int>(user & counter_max);
    infos->at(i).idle = static_cast<int>(idle & counter_max);
    infos->at(i).total = static_cast<int>((kernel + user) & counter_max);
  }

  return true;
}