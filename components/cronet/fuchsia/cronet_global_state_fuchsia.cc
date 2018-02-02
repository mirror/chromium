// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cronet/cronet_global_state.h"

#include "base/at_exit.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_scheduler.h"
#include "net/proxy_resolution/proxy_config_service.h"
#include "net/proxy_resolution/proxy_service.h"
#include "url/url_util.h"

namespace cronet {

namespace {

base::SingleThreadTaskRunner* g_init_task_runner = nullptr;

// TODO(wez): Remove this once AtExitManager dependencies are gone.
base::AtExitManager* g_at_exit_manager = nullptr;

}  // namespace

void EnsureInitialized() {
  if (g_init_task_runner)
    return;

  url::Initialize();
  base::TaskScheduler::CreateAndStartWithDefaultParams("cronet");

  // TODO(wez): Remove this once AtExitManager dependencies are gone.
  g_at_exit_manager = new base::AtExitManager;

  // scoped_refptr<> won't let us release() and leak a bare reference, so
  // we have to do a little dance to shoot ourself in the foot.
  scoped_refptr<base::SingleThreadTaskRunner> init_task_runner =
      base::CreateSingleThreadTaskRunnerWithTraits({});
  init_task_runner->AddRef();
  g_init_task_runner = init_task_runner.get();
}

bool OnInitThread() {
  return g_init_task_runner->BelongsToCurrentThread();
}

void PostTaskToInitThread(const base::Location& posted_from,
                          base::OnceClosure task) {
  g_init_task_runner->PostTask(posted_from, std::move(task));
}

std::unique_ptr<net::ProxyConfigService> CreateProxyConfigService(
    const scoped_refptr<base::SequencedTaskRunner>& io_task_runner) {
  NOTIMPLEMENTED();
  return nullptr;
}

std::unique_ptr<net::ProxyResolutionService> CreateProxyService(
    std::unique_ptr<net::ProxyConfigService> proxy_config_service,
    net::NetLog* net_log) {
  NOTIMPLEMENTED();
  return nullptr;
}

std::string CreateDefaultUserAgent(const std::string& partial_user_agent) {
  return partial_user_agent;
}

}  // namespace cronet
