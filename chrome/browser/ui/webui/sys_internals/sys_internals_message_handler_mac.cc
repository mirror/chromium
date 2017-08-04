// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/sys_internals/sys_internals_message_handler.h"

#include <mach/mach_host.h>

#include "base/mac/scoped_mach_port.h"
#include "base/sys_info.h"

bool SysInternalsMessageHandler::GetCpuInfo(std::vector<base::CpuInfo>* infos) {
  DCHECK(infos);

  natural_t num_of_processors;
  base::mac::ScopedMachSendRight host(mach_host_self());
  mach_msg_type_number_t type;
  processor_cpu_load_info_data_t* cpu_infos;

  if (host_processor_info(host.get(), PROCESSOR_CPU_LOAD_INFO,
                          &num_of_processors,
                          reinterpret_cast<processor_info_array_t*>(&cpu_infos),
                          &type) == KERN_SUCCESS) {
    DCHECK_EQ(num_of_processors,
              static_cast<natural_t>(base::SysInfo::NumberOfProcessors()));
    DCHECK_EQ(num_of_processors, static_cast<natural_t>(infos->size()));

    for (natural_t i = 0; i < num_of_processors; ++i) {
      uint64_t user =
          static_cast<uint64_t>(cpu_infos[i].cpu_ticks[CPU_STATE_USER]);
      uint64_t sys =
          static_cast<uint64_t>(cpu_infos[i].cpu_ticks[CPU_STATE_SYSTEM]);
      uint64_t nice =
          static_cast<uint64_t>(cpu_infos[i].cpu_ticks[CPU_STATE_NICE]);
      uint64_t idle =
          static_cast<uint64_t>(cpu_infos[i].cpu_ticks[CPU_STATE_IDLE]);

      uint32_t counter_max = SysInternalsMessageHandler::COUNTER_MAX;
      infos->at(i).kernel = static_cast<int>(sys & counter_max);
      infos->at(i).user = static_cast<int>((user + nice) & counter_max);
      infos->at(i).idle = static_cast<int>(idle & counter_max);
      infos->at(i).total =
          static_cast<int>((sys + user + nice + idle) & counter_max);
    }

    vm_deallocate(mach_task_self(), reinterpret_cast<vm_address_t>(cpu_infos),
                  num_of_processors * sizeof(processor_cpu_load_info));

    return true;
  }

  return false;
}
