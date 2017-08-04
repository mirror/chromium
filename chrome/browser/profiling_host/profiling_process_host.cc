// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/profiling_host/profiling_process_host.h"

#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "build/build_config.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/profiling/constants.mojom.h"
#include "chrome/common/profiling/profiling_constants.h"
#include "content/public/browser/browser_child_process_host.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/bind_interface_helpers.h"
#include "content/public/common/child_process_host.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/service_manager_connection.h"
#include "mojo/edk/embedder/platform_channel_pair.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"
#include "mojo/public/cpp/system/platform_handle.h"

namespace profiling {

namespace {

void BindToBrowserConnector(service_manager::mojom::ConnectorRequest request) {
  if (!content::BrowserThread::CurrentlyOn(content::BrowserThread::UI)) {
    content::BrowserThread::PostTask(
        content::BrowserThread::UI, FROM_HERE,
        base::BindOnce(&BindToBrowserConnector, std::move(request)));
    return;
  }
  content::ServiceManagerConnection::GetForProcess()
      ->GetConnector()
      ->BindConnectorRequest(std::move(request));
}

}  // namespace

ProfilingProcessHost::ProfilingProcessHost() {
  Add(this);
  registrar_.Add(this, content::NOTIFICATION_RENDERER_PROCESS_CREATED,
                 content::NotificationService::AllBrowserContextsAndSources());
}

ProfilingProcessHost::~ProfilingProcessHost() {
  Remove(this);
}

void ProfilingProcessHost::BrowserChildProcessLaunchedAndConnected(
    const content::ChildProcessData& data) {
  if (!content::BrowserThread::CurrentlyOn(content::BrowserThread::IO)) {
    content::BrowserThread::GetTaskRunnerForThread(content::BrowserThread::IO)
        ->PostTask(
            FROM_HERE,
            base::BindOnce(
                &ProfilingProcessHost::BrowserChildProcessLaunchedAndConnected,
                base::Unretained(this), data));
    return;
  }
  content::BrowserChildProcessHost* host =
      content::BrowserChildProcessHost::FromID(data.id);
  if (!host)
    return;
  profiling::mojom::MemlogClientPtr memlog_client;
  profiling::mojom::MemlogClientRequest request =
      mojo::MakeRequest(&memlog_client);
  BindInterface(host->GetHost(), std::move(request));
  StartProfilingForClient(std::move(memlog_client));
}

void ProfilingProcessHost::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  if (!content::BrowserThread::CurrentlyOn(content::BrowserThread::IO)) {
    content::BrowserThread::GetTaskRunnerForThread(content::BrowserThread::IO)
        ->PostTask(FROM_HERE, base::BindOnce(&ProfilingProcessHost::Observe,
                                             base::Unretained(this), type,
                                             source, details));
    return;
  }
  if (type != content::NOTIFICATION_RENDERER_PROCESS_CREATED)
    return;
  content::RenderProcessHost* host =
      content::Source<content::RenderProcessHost>(source).ptr();
  profiling::mojom::MemlogClientPtr memlog_client;
  profiling::mojom::MemlogClientRequest request =
      mojo::MakeRequest(&memlog_client);
  host->BindInterface(profiling::mojom::MemlogClient::Name_,
                      request.PassMessagePipe());

  StartProfilingForClient(std::move(memlog_client));
}

void ProfilingProcessHost::StartProfilingForClient(
    profiling::mojom::MemlogClientPtr memlog_client) {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::IO));

  mojo::edk::PlatformChannelPair data_channel;
  memlog_->AddSender(
      mojo::WrapPlatformFile(data_channel.PassServerHandle().release().handle),
      next_sender_id_++);
  memlog_client->StartProfiling(
      mojo::WrapPlatformFile(data_channel.PassClientHandle().release().handle));
}

// static
ProfilingProcessHost* ProfilingProcessHost::EnsureStarted(
    content::ServiceManagerConnection* connection) {
  ProfilingProcessHost* host = GetInstance();
  host->MakeConnector(connection);
  host->LaunchAsService();
  return host;
}

// static
ProfilingProcessHost* ProfilingProcessHost::GetInstance() {
  return base::Singleton<
      ProfilingProcessHost,
      base::LeakySingletonTraits<ProfilingProcessHost>>::get();
}

// static
void ProfilingProcessHost::AddSwitchesToChildCmdLine(
    base::CommandLine* child_cmd_line) {
  // TODO(ajwong): Figure out how to trace the zygote process.
  if (child_cmd_line->GetSwitchValueASCII(switches::kProcessType) ==
      switches::kZygoteProcess) {
    return;
  }

  // TODO(ajwong): Change this to just reuse the --memlog flag. There is no
  // need for a separate pipe flag.
  //
  // Zero is browser which is specified in LaunchAsService.
  static int sender_id = 1;
  child_cmd_line->AppendSwitchASCII(switches::kMemlogPipe,
                                    base::IntToString(sender_id++));
}

void ProfilingProcessHost::RequestProcessDump(base::ProcessId pid) {
  // TODO(brettw) implement process dumping.
}

void ProfilingProcessHost::MakeConnector(
    content::ServiceManagerConnection* connection) {
  connector_ = connection->GetConnector()->Clone();
}

void ProfilingProcessHost::LaunchAsService() {
  // May get called on different threads, we need to be on the IO thread to
  // work.
  //
  // TODO(ajwong): This thread bouncing logic is dumb. The
  // BindToBrowserConnector() ends up jumping to the UI thread also so this is
  // at least 2 bounces. Simplify.
  if (!content::BrowserThread::CurrentlyOn(content::BrowserThread::IO)) {
    content::BrowserThread::GetTaskRunnerForThread(content::BrowserThread::IO)
        ->PostTask(FROM_HERE,
                   base::BindOnce(&ProfilingProcessHost::LaunchAsService,
                                  base::Unretained(this)));
    return;
  }

  connector_->BindInterface(mojom::kServiceName, &memlog_);

  mojo::edk::PlatformChannelPair data_channel;
  memlog_->AddSender(
      mojo::WrapPlatformFile(data_channel.PassServerHandle().release().handle),
      next_sender_id_++);  // 0 is the browser.
  memlog_client_.StartProfiling(
      mojo::WrapPlatformFile(data_channel.PassClientHandle().release().handle));
}

}  // namespace profiling
