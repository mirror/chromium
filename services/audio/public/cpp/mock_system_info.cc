// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/audio/public/cpp/mock_system_info.h"

#include "base/single_thread_task_runner.h"
#include "media/audio/audio_manager.h"
#include "services/audio/common/system_info.h"
#include "services/audio/public/interfaces/constants.mojom.h"
#include "services/audio/public/interfaces/system_info.mojom.h"
#include "services/service_manager/public/cpp/bind_source_info.h"
#include "services/service_manager/public/cpp/service_context.h"

namespace audio {

MockSystemInfo::MockSystemInfo(media::AudioManager* audio_manager)
    : audio_manager_(audio_manager) {
  DCHECK(audio_manager_);
}

MockSystemInfo::~MockSystemInfo() {
  DCHECK(GetTaskRunner()->BelongsToCurrentThread());
}

// static
void MockSystemInfo::OverrideGlobalBinder(MockSystemInfo* mock_system_info) {
  service_manager::ServiceContext::SetGlobalBinderForTesting(
      mojom::kServiceName, mojom::SystemInfo::Name_,
      base::BindRepeating(&MockSystemInfo::Bind,
                          base::Unretained(mock_system_info)),
      mock_system_info->GetTaskRunner());
}

void MockSystemInfo::Bind(const std::string& interface_name,
                          mojo::ScopedMessagePipeHandle handle,
                          const service_manager::BindSourceInfo& source_info) {
  DCHECK(interface_name == mojom::SystemInfo::Name_);
  DCHECK(GetTaskRunner()->BelongsToCurrentThread());
  if (!system_info_)
    system_info_ = std::make_unique<SystemInfo>(audio_manager_);
  system_info_->Bind(mojom::SystemInfoRequest(std::move(handle)));
}

base::SingleThreadTaskRunner* MockSystemInfo::GetTaskRunner() {
  return audio_manager_->GetTaskRunner();
}

}  // namespace audio
