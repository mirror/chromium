// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <iostream>

#include "base/callback.h"
#include "base/command_line.h"
#include "base/files/file.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/prefs/in_memory_pref_store.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/pref_service_factory.h"
#include "components/subresource_filter/core/browser/ruleset_service.h"
#include "components/subresource_filter/core/browser/ruleset_service_delegate.h"

namespace {

const char kHelpMsg[] = R"(
  servicetool <unindexed_ruleset_file>

  Servicetool will open the unindexed_ruleset_file and output an indexed file (in the appropriate subdirectory).
)";

void PrintHelp() {
  std::cout << kHelpMsg << std::endl << std::endl;
}

class Delegate : public subresource_filter::RulesetServiceDelegate {
 public:
  Delegate() = default;
  ~Delegate() override = default;

  void PostAfterStartupTask(base::Closure task) override { task.Run(); }
  void PublishNewRulesetVersion(base::File ruleset_data) override { exit(0); }

 private:
  DISALLOW_COPY_AND_ASSIGN(Delegate);
};

}  // anonymous namespace

// Runs on the main thread
int main(int argc, char* argv[]) {
  base::CommandLine::Init(argc, argv);
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  base::CommandLine::StringVector args = command_line.GetArgs();

  if (args.size() < 1U) {
    PrintHelp();
    return 1;
  }

  base::MessageLoopForIO message_loop;

  scoped_refptr<PrefRegistrySimple> pref_registry(new PrefRegistrySimple());
  scoped_refptr<InMemoryPrefStore> local_state_store(new InMemoryPrefStore());
  PrefServiceFactory prefs_factory;
  prefs_factory.set_user_prefs(local_state_store);
  std::unique_ptr<PrefService> pref_service =
      prefs_factory.Create(pref_registry.get());

  Delegate ruleset_delegate;
  subresource_filter::RulesetService service(
      pref_service.get(), base::ThreadTaskRunnerHandle::Get(),
      &ruleset_delegate, base::FilePath());

  subresource_filter::UnindexedRulesetInfo ruleset_info;
  ruleset_info.content_version = "1";
  ruleset_info.ruleset_path = base::FilePath(args[0]);

  service.IndexAndStoreAndPublishRulesetIfNeeded(ruleset_info);

  base::RunLoop().Run();
}
