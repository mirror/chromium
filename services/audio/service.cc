// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/audio/service.h"

#include "base/macros.h"
#include "base/single_thread_task_runner.h"
#include "build/build_config.h"
#include "media/audio/audio_manager.h"
#include "services/audio/system_info.h"
#include "services/service_manager/public/cpp/service_context.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

namespace audio {

Service::Service(std::unique_ptr<AudioManagerAccessor> audio_manager_accessor,
                 base::TimeDelta quit_timeout)
    : quit_timeout_(quit_timeout),
      audio_manager_accessor_(std::move(audio_manager_accessor)),
      weak_factory_(this) {
  DCHECK(audio_manager_accessor_);
}

Service::~Service() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
}

void Service::OnStart() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  ref_factory_ = std::make_unique<service_manager::ServiceContextRefFactory>(
      base::BindRepeating(&Service::MaybeRequestQuitDelayed,
                          base::Unretained(this)));
  registry_.AddInterface<mojom::SystemInfo>(base::BindRepeating(
      &Service::BindSystemInfoRequest, base::Unretained(this)));
}

void Service::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}

bool Service::OnServiceManagerConnectionLost() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  audio_manager_accessor_->Shutdown();
  return true;
}

void Service::SetQuitClosureForTesting(base::RepeatingClosure quit_closure) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  quit_closure_ = std::move(quit_closure);
}

void Service::BindSystemInfoRequest(mojom::SystemInfoRequest request) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (!system_info_) {
    system_info_ = std::make_unique<SystemInfo>(
        audio_manager_accessor_->GetAudioManager());
  }
  system_info_->Bind(std::move(request), ref_factory_->CreateRef());
}

void Service::MaybeRequestQuitDelayed() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (quit_timeout_ <= base::TimeDelta())
    return;
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&Service::MaybeRequestQuit, weak_factory_.GetWeakPtr()),
      quit_timeout_);
}

void Service::MaybeRequestQuit() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(ref_factory_);
  if (ref_factory_->HasNoRefs()) {
    context()->RequestQuit();
    if (!quit_closure_.is_null())
      std::move(quit_closure_).Run();
  }
}

}  // namespace audio
