// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_AUDIO_PUBLIC_CPP_MOCK_SYSTEM_INFO_H_
#define SERVICES_AUDIO_PUBLIC_CPP_MOCK_SYSTEM_INFO_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "mojo/public/cpp/system/message_pipe.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace media {
class AudioManager;
}

namespace service_manager {
struct BindSourceInfo;
}

namespace audio {
class SystemInfo;

class MockSystemInfo {
 public:
  MockSystemInfo(media::AudioManager* audio_manager);

  // Must be called on the device task runner of |audio_manager|.
  ~MockSystemInfo();

  static void OverrideGlobalBinder(MockSystemInfo* mock_system_info);

 private:
  void Bind(const std::string& interface_name,
            mojo::ScopedMessagePipeHandle handle,
            const service_manager::BindSourceInfo& source_info);

  base::SingleThreadTaskRunner* GetTaskRunner();

  media::AudioManager* audio_manager_;
  std::unique_ptr<SystemInfo> system_info_;

  DISALLOW_COPY_AND_ASSIGN(MockSystemInfo);
};

}  // namespace audio

#endif  // SERVICES_AUDIO_PUBLIC_CPP_MOCK_SYSTEM_INFO_H_
