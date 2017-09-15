// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/keyboard_lock/host.h"

#include <memory>
#include <utility>

#include "base/memory/singleton.h"
#include "content/browser/keyboard_lock/client.h"
#include "content/browser/keyboard_lock/page_observer.h"
#include "content/public/browser/browser_thread.h"

namespace content {
namespace keyboard_lock {

// static
Host* Host::GetInstance() {
  return base::Singleton<Host>::get();
}

Host::Host()
    : BaseHost(BrowserThread::GetTaskRunnerForThread(BrowserThread::UI)) {}
Host::~Host() = default;

std::unique_ptr<ui::aura::keyboard_lock::Client> Host::CreateClient(
    content::WebContents* const& client) {
  return Client::Create(client);
}

void Host::ObserveClient(content::WebContents* const& client,
                         std::unique_ptr<ObserverType> observer) {
  new PageObserver(client, std::move(observer));
}

}  // namespace keyboard_lock
}  // namespace content
