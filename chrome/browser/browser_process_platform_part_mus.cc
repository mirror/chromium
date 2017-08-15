// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browser_process_platform_part_mus.h"

#include "content/public/browser/discardable_shared_memory_manager.h"
#include "services/ui/common/image_cursors_set.h"
#include "services/ui/public/interfaces/constants.mojom.h"
#include "services/ui/service.h"

#if defined(OS_CHROMEOS)
#include "ash/public/interfaces/constants.mojom.h"
#include "chrome/browser/chromeos/ash_config.h"
#include "chrome/browser/prefs/active_profile_pref_service.h"
#include "chrome/browser/ui/ash/ash_util.h"
#endif

namespace {

std::unique_ptr<service_manager::Service> CreateEmbeddedUIService(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    base::WeakPtr<ui::ImageCursorsSet> image_cursors_set_weak_ptr,
    discardable_memory::DiscardableSharedMemoryManager* memory_manager) {
  ui::Service::InProcessConfig config;
  config.resource_runner = task_runner;
  config.image_cursors_set_weak_ptr = image_cursors_set_weak_ptr;
  config.memory_manager = memory_manager;
  return base::MakeUnique<ui::Service>(&config);
}

}  // namespace

BrowserProcessPlatformPartMus::BrowserProcessPlatformPartMus() {}

BrowserProcessPlatformPartMus::~BrowserProcessPlatformPartMus() {}

void BrowserProcessPlatformPartMus::RegisterInProcessServices(
    content::ContentBrowserClient::StaticServiceMap* services) {
#if defined(OS_CHROMEOS)
  {
    service_manager::EmbeddedServiceInfo info;
    info.factory = base::Bind([] {
      return std::unique_ptr<service_manager::Service>(
          base::MakeUnique<ActiveProfilePrefService>());
    });
    info.task_runner = base::ThreadTaskRunnerHandle::Get();
    services->insert(std::make_pair(prefs::mojom::kForwarderServiceName, info));
  }

  if (!ash_util::IsRunningInMash()) {
    service_manager::EmbeddedServiceInfo info;
    info.factory = base::Bind(&ash_util::CreateEmbeddedAshService,
                              base::ThreadTaskRunnerHandle::Get());
    info.task_runner = base::ThreadTaskRunnerHandle::Get();
    services->insert(std::make_pair(ash::mojom::kServiceName, info));
  }
  if (chromeos::GetAshConfig() != ash::Config::MUS)
    return;
#endif

  service_manager::EmbeddedServiceInfo info;
  image_cursors_set_ = base::MakeUnique<ui::ImageCursorsSet>();
  info.factory =
      base::Bind(&CreateEmbeddedUIService, base::ThreadTaskRunnerHandle::Get(),
                 image_cursors_set_->GetWeakPtr(),
                 content::GetDiscardableSharedMemoryManager());
  info.use_own_thread = true;
  info.message_loop_type = base::MessageLoop::TYPE_UI;
  info.thread_priority = base::ThreadPriority::DISPLAY;
  services->insert(std::make_pair(ui::mojom::kServiceName, info));
}

void BrowserProcessPlatformPartMus::DestroyImageCursorsSet() {
  image_cursors_set_.reset();
}
