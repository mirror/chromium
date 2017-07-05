// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/web/public/global_state/ios_global_state.h"

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/task_scheduler/initialization_util.h"
#include "components/task_scheduler_util/browser/initialization.h"
#include "net/base/network_change_notifier.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

base::TaskScheduler::InitParams GetDefaultTaskSchedulerInitParams() {
  using StandbyThreadPolicy =
      base::SchedulerWorkerPoolParams::StandbyThreadPolicy;
  return base::TaskScheduler::InitParams(
      base::SchedulerWorkerPoolParams(
          StandbyThreadPolicy::ONE,
          base::RecommendedMaxNumberOfThreadsInPool(2, 8, 0.1, 0),
          base::TimeDelta::FromSeconds(30)),
      base::SchedulerWorkerPoolParams(
          StandbyThreadPolicy::ONE,
          base::RecommendedMaxNumberOfThreadsInPool(2, 8, 0.1, 0),
          base::TimeDelta::FromSeconds(30)),
      base::SchedulerWorkerPoolParams(
          StandbyThreadPolicy::ONE,
          base::RecommendedMaxNumberOfThreadsInPool(3, 8, 0.3, 0),
          base::TimeDelta::FromSeconds(30)),
      base::SchedulerWorkerPoolParams(
          StandbyThreadPolicy::ONE,
          base::RecommendedMaxNumberOfThreadsInPool(3, 8, 0.3, 0),
          base::TimeDelta::FromSeconds(60)));
}

}  // namespace

namespace ios_global_state {

void Create(const CreateParams& create_params) {
  static std::unique_ptr<base::AtExitManager> exit_manager;

  static dispatch_once_t once_token;
  dispatch_once(&once_token, ^{
    if (create_params.install_at_exit_manager) {
      exit_manager = base::MakeUnique<base::AtExitManager>();
    }

    // Use an empty string as TaskScheduler name to match the suffix of browser
    // process TaskScheduler histograms.
    base::TaskScheduler::Create("");

    base::CommandLine::Init(create_params.argc, create_params.argv);
  });
}

void BuildMessageLoop() {
  static std::unique_ptr<base::MessageLoopForUI> main_message_loop;

  static dispatch_once_t once_token;
  dispatch_once(&once_token, ^{
    // Create a MessageLoop if one does not already exist for the current
    // thread.
    if (!base::MessageLoop::current()) {
      main_message_loop = base::MakeUnique<base::MessageLoopForUI>();
    }
    base::MessageLoopForUI::current()->Attach();
  });
}

void CreateNetworkChangeNotifier() {
  static std::unique_ptr<net::NetworkChangeNotifier> network_change_notifier;

  static dispatch_once_t once_token;
  dispatch_once(&once_token, ^{
    network_change_notifier.reset(net::NetworkChangeNotifier::Create());
  });
}

void StartTaskScheduler() {
  static dispatch_once_t once_token;
  dispatch_once(&once_token, ^{
    auto init_params_from_variations =
        task_scheduler_util::GetBrowserTaskSchedulerInitParamsFromVariations();
    if (init_params_from_variations) {
      base::TaskScheduler::GetInstance()->Start(*init_params_from_variations);
    } else {
      base::TaskScheduler::GetInstance()->Start(
          GetDefaultTaskSchedulerInitParams());
    }
  });
}

}  // namespace ios_global_state
