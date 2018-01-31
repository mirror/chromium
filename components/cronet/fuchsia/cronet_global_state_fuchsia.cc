// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cronet/cronet_global_state.h"

#include "base/at_exit.h"
#include "base/task_scheduler/task_scheduler.h"
#include "base/threading/platform_thread.h"
#include "base/threading/thread.h"
#include "net/proxy_resolution/proxy_config_service.h"
#include "net/proxy_resolution/proxy_service.h"
#include "url/url_util.h"

namespace cronet {

namespace {

base::Thread* InitThread() {
  // TODO(jamesr): Is this thread supposed to go away at some point?
  static base::Thread* init_thread = new base::Thread("cronet_init");
  return init_thread;
}

}  // namespace

void EnsureInitialized() {
  url::Initialize();
  base::TaskScheduler::CreateAndStartWithDefaultParams("cronet");
  static base::AtExitManager* at_exit = new base::AtExitManager();
  (void)at_exit;
}

bool OnInitThread() {
  return InitThread()->GetThreadId() == base::PlatformThread::CurrentId();
}

void PostTaskToInitThread(const base::Location& posted_from,
                          base::OnceClosure task) {
  InitThread()->Start();
  InitThread()->task_runner()->PostTask(posted_from, std::move(task));
}

std::unique_ptr<net::ProxyConfigService> CreateProxyConfigService(
    const scoped_refptr<base::SequencedTaskRunner>& io_task_runner) {
  return nullptr;
}

std::unique_ptr<net::ProxyResolutionService> CreateProxyService(
    std::unique_ptr<net::ProxyConfigService> proxy_config_service,
    net::NetLog* net_log) {
  return nullptr;
}

std::string CreateDefaultUserAgent(const std::string& partial_user_agent) {
  return partial_user_agent;
}

}  // namespace cronet
