// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/content/test/test_utils.h"

#include "base/memory/ptr_util.h"
#include "components/download/content/internal/download_driver_impl.h"
#include "components/download/internal/client_set.h"
#include "components/download/internal/config.h"
#include "components/download/internal/controller_impl.h"
#include "components/download/internal/download_service_impl.h"
#include "components/download/internal/download_store.h"
#include "components/download/internal/file_monitor_impl.h"
#include "components/download/internal/model_impl.h"
#include "components/download/internal/proto/entry.pb.h"
#include "components/download/internal/scheduler/scheduler_impl.h"
#include "components/download/internal/test/test_device_status_listener.h"
#include "components/leveldb_proto/proto_database_impl.h"

namespace download {
namespace test {

DownloadService* CreateDownloadServiceForTest(
    std::unique_ptr<DownloadClientMap> clients,
    content::BrowserContext* browser_context,
    const base::FilePath& files_storage_dir,
    const base::FilePath& db_storage_dir,
    const scoped_refptr<base::SequencedTaskRunner>& background_task_runner,
    std::unique_ptr<TaskScheduler> task_scheduler,
    NavigationMonitor* navigation_monitor) {
  auto client_set = base::MakeUnique<ClientSet>(std::move(clients));
  auto config = Configuration::CreateFromFinch();
  auto driver = base::MakeUnique<DownloadDriverImpl>(
      content::BrowserContext::GetDownloadManager(browser_context));

  auto entry_db =
      base::MakeUnique<leveldb_proto::ProtoDatabaseImpl<protodb::Entry>>(
          background_task_runner);
  auto store =
      base::MakeUnique<DownloadStore>(db_storage_dir, std::move(entry_db));
  auto model = base::MakeUnique<ModelImpl>(std::move(store));

  // Bypass Android implementation, which is built on JNI bridge and cannot be
  // used in browser test.
  auto device_status_listener =
      base::MakeUnique<TestDeviceStatusListener>(config->network_change_delay);
  auto scheduler = base::MakeUnique<SchedulerImpl>(
      task_scheduler.get(), config.get(), client_set.get());
  auto file_monitor = base::MakeUnique<FileMonitorImpl>(
      files_storage_dir, background_task_runner, config->file_keep_alive_time);
  auto controller = base::MakeUnique<ControllerImpl>(
      config.get(), std::move(client_set), std::move(driver), std::move(model),
      std::move(device_status_listener), navigation_monitor,
      std::move(scheduler), std::move(task_scheduler), std::move(file_monitor),
      files_storage_dir);
  return new DownloadServiceImpl(std::move(config), std::move(controller));
}

}  // namespace test
}  // namespace download
