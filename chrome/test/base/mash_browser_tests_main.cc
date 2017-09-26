// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/debug/debugger.h"
#include "base/debug/stack_trace.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/i18n/icu_util.h"
#include "base/json/json_reader.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/path_service.h"
#include "base/process/launch.h"
#include "base/process/process_handle.h"
#include "base/run_loop.h"
#include "base/sys_info.h"
#include "base/task_scheduler/task_scheduler.h"
#include "base/test/launcher/test_launcher.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/values.h"
#include "chrome/app/mash/embedded_services.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/chrome_test_launcher.h"
#include "chrome/test/base/chrome_test_suite.h"
#include "chrome/test/base/mojo_test_connector.h"
#include "content/public/app/content_main.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/service_manager_connection.h"
#include "content/public/common/service_names.mojom.h"
#include "content/public/test/test_launcher.h"
#include "mash/session/public/interfaces/constants.mojom.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/public/cpp/service_context.h"
#include "services/service_manager/public/cpp/service_runner.h"
#include "services/service_manager/public/cpp/standalone_service/standalone_service.h"
#include "services/service_manager/runner/common/switches.h"
#include "services/service_manager/runner/init.h"
#include "services/ui/common/switches.h"
#include "ui/aura/env.h"

namespace {

// XXX remove files!
/*
const base::FilePath::CharType kMashCatalogFilename[] =
    FILE_PATH_LITERAL("mash_browser_tests_catalog.json");
const base::FilePath::CharType kMusCatalogFilename[] =
    FILE_PATH_LITERAL("mus_browser_tests_catalog.json");
*/

// ChromeTestLauncherDelegate implementation used for '--mus' (ash runs in
// process with chrome).
class MusTestLauncherDelegate : public ChromeTestLauncherDelegate {
 public:
  MusTestLauncherDelegate(ChromeTestSuiteRunner* runner)
      : ChromeTestLauncherDelegate(runner) {
    base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
    config_ = command_line->HasSwitch(switches::kMash)
                  ? MojoTestConnector::Config::MASH
                  : MojoTestConnector::Config::MUS;
  }
  ~MusTestLauncherDelegate() override {}

 private:
  // ChromeTestLauncherDelegate:
  int RunTestSuite(int argc, char** argv) override {
    content::GetContentMainParams()->env_mode = aura::Env::Mode::MUS;
    content::GetContentMainParams()->create_discardable_memory =
        (config_ == MojoTestConnector::Config::MUS);
    return ChromeTestLauncherDelegate::RunTestSuite(argc, argv);
  }

  MojoTestConnector::Config config_;

  DISALLOW_COPY_AND_ASSIGN(MusTestLauncherDelegate);
};

}  // namespace

bool RunMashBrowserTests(int argc, char** argv, int* exit_code) {
  base::CommandLine::Init(argc, argv);
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  LOG(ERROR) << "RunMashBrowserTests cl="
             << command_line->GetCommandLineString()
             << " pid=" << base::GetCurrentProcId();
  if (!command_line->HasSwitch(switches::kMash) &&
      !command_line->HasSwitch(switches::kMus)) {
    // Currently launching content_package_services via the browser_tests binary
    // will lead to a nested test suite, trying to run all tests again. However
    // they will be in a strange mixed mode, of a local-Ash, but non-local Aura.
    //
    // This leads to continuous crashes in OzonePlatform.
    //
    // For now disable this launch until the requesting site can be identified.
    //
    // TODO(jonross): find an appropriate way to launch content_package_services
    // within the mash_browser_tests (crbug.com/738449)
    if (command_line->GetSwitchValueASCII("service-name") ==
        content::mojom::kPackagedServicesServiceName) {
      return true;
    }
    return false;
  }

  if (command_line->HasSwitch(switches::kProcessType)) {
    LOG(ERROR) << "Bailing on service";
    return false;
  }

  size_t parallel_jobs = base::NumParallelJobs();
  ChromeTestSuiteRunner chrome_test_suite_runner;
  MusTestLauncherDelegate test_launcher_delegate(&chrome_test_suite_runner);
  if (command_line->HasSwitch(switches::kMash) && parallel_jobs > 1U)
    parallel_jobs /= 2U;

  *exit_code =
      LaunchChromeTests(parallel_jobs, &test_launcher_delegate, argc, argv);
  return true;
}
