// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/chrome_ssl_config_tracker.h"

#include "chrome/browser/browser_process.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "components/ssl_config/ssl_config_service_manager.h"
#include "content/public/browser/browser_thread.h"

ChromeSSLConfigTracker::ChromeSSLConfigTracker(
    content::mojom::SSLConfigClientPtr ssl_config_client)
    : ssl_config_service_manager_(
          ssl_config::SSLConfigServiceManager::CreateDefaultManager(
              g_browser_process->local_state(),
              // TODO(mmenke): Can SSLConfigServiceManager be modified to use a
              // SequencedTaskRunner instead of a SingleThreadedTaskRunner?
              content::BrowserThread::GetTaskRunnerForThread(
                  content::BrowserThread::UI))),
      ssl_config_client_(std::move(ssl_config_client)) {
  ssl_config_service_manager_->Get()->AddObserver(this);
}

ChromeSSLConfigTracker::~ChromeSSLConfigTracker() {
  ssl_config_service_manager_->Get()->RemoveObserver(this);
}

net::SSLConfig ChromeSSLConfigTracker::GetSSLConfig() const {
  net::SSLConfig ssl_config;
  ssl_config_service_manager_->Get()->GetSSLConfig(&ssl_config);
  return ssl_config;
}

void ChromeSSLConfigTracker::OnSSLConfigChanged() {
  ssl_config_client_->UpdateSSLConfig(GetSSLConfig());
}
