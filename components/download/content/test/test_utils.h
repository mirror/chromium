// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_CONTENT_TEST_TEST_UTILS_H_
#define COMPONENTS_DOWNLOAD_CONTENT_TEST_TEST_UTILS_H_

#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/sequenced_task_runner.h"
#include "components/download/public/clients.h"
#include "components/keyed_service/core/keyed_service.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace download {

class DownloadService;
class NavigationMonitor;
class TaskScheduler;

namespace test {

DownloadService* CreateDownloadServiceForTest(
    std::unique_ptr<DownloadClientMap> clients,
    content::BrowserContext* browser_context,
    const base::FilePath& files_storage_dir,
    const base::FilePath& db_storage_dir,
    const scoped_refptr<base::SequencedTaskRunner>& background_task_runner,
    std::unique_ptr<TaskScheduler> task_scheduler,
    NavigationMonitor* navigation_monitor);

}  // namespace test
}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_CONTENT_TEST_TEST_UTILS_H_
