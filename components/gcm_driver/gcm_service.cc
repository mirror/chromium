// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/gcm_driver/gcm_service.h"
#include "components/gcm_driver/gcm_client.h"
#include "components/gcm_driver/gcm_client_factory.h"
#include "components/gcm_driver/gcm_driver_desktop.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/pref_service_factory.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/files/file_path.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/url_request/url_request_context_getter.h"

namespace gcm {

namespace {

const char kFakeChannelStatusRequestUrl[] =
    "http:://channel.status.request.url";

}  // namespace

GCMService::GCMService() : task_runner_(base::ThreadTaskRunnerHandle::Get()) {}

GCMService::~GCMService() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  gcm_driver_->Shutdown();
}

void GCMService::CreateGCMDriver(
    const GCMClient::ChromeBuildInfo& chrome_build_info,
    PrefService* pref_service,
    net::URLRequestContextGetter* url_context_getter,
    const scoped_refptr<base::SingleThreadTaskRunner>& ui_task_runner,
    const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner,
    const scoped_refptr<base::SingleThreadTaskRunner>& blocking_task_runner) {
  DCHECK(!gcm_driver_);
  gcm_driver_.reset(new GCMDriverDesktop(
      base::MakeUnique<GCMClientFactory>(), chrome_build_info,
      kFakeChannelStatusRequestUrl, "" /* user_agent */, pref_service,
      base::FilePath("") /* store path */, url_context_getter, ui_task_runner,
      io_task_runner, blocking_task_runner));
  // gcm_driver_->AddConnectionObserver();
  gcm_driver_->Enable();
  gcm_driver_->OnSignedIn();
}

}  // namespace gcm
