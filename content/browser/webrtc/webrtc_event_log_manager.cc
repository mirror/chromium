// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webrtc/webrtc_event_log_manager.h"

#include "base/task_scheduler/post_task.h"
#include "content/common/media/peer_connection_tracker_messages.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"

namespace content {

namespace {

class PeerConnectionTrackerProxyImpl
    : public WebRtcEventLogManager::PeerConnectionTrackerProxy {
 public:
  ~PeerConnectionTrackerProxyImpl() override = default;

  void StartEventLogOutput(WebRtcEventLogPeerConnectionKey key) override {
    RenderProcessHost* host = RenderProcessHost::FromID(key.render_process_id);
    if (!host) {
      return;  // The host has been asynchronously removed; not a problem.
    }
    host->Send(new PeerConnectionTracker_StartEventLogOutput(key.lid));
  }

  void StopEventLogOutput(WebRtcEventLogPeerConnectionKey key) override {
    RenderProcessHost* host = RenderProcessHost::FromID(key.render_process_id);
    if (!host) {
      return;  // The host has been asynchronously removed; not a problem.
    }
    host->Send(new PeerConnectionTracker_StopEventLog(key.lid));
  }
};

const BrowserContext* GetBrowserContext(int render_process_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  RenderProcessHost* host = RenderProcessHost::FromID(render_process_id);
  // TODO: !!! Death tests where applicable?
  CHECK(host);
  return host->GetBrowserContext();
}

}  // namespace

const size_t kWebRtcEventLogManagerUnlimitedFileSize = 0;

WebRtcEventLogManager* WebRtcEventLogManager::g_webrtc_event_log_manager =
    nullptr;

WebRtcEventLogManager* WebRtcEventLogManager::CreateSingletonInstance() {
  DCHECK(!g_webrtc_event_log_manager);
  g_webrtc_event_log_manager = new WebRtcEventLogManager;
  return g_webrtc_event_log_manager;
}

WebRtcEventLogManager* WebRtcEventLogManager::GetInstance() {
  return g_webrtc_event_log_manager;
}

WebRtcEventLogManager::WebRtcEventLogManager()
    : local_logs_observer_(nullptr),
      remote_logs_observer_(nullptr),
      local_logs_manager_(this),
      remote_logs_manager_(this),
      pc_tracker_proxy_(new PeerConnectionTrackerProxyImpl),
      task_runner_(base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND,
           base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN})) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(!g_webrtc_event_log_manager);
  g_webrtc_event_log_manager = this;
}

WebRtcEventLogManager::~WebRtcEventLogManager() {
  DCHECK(g_webrtc_event_log_manager);
  g_webrtc_event_log_manager = nullptr;
}

void WebRtcEventLogManager::OnBrowserContextInitialized(
    const BrowserContext* browser_context,
    base::OnceClosure reply) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(
          &WebRtcEventLogManager::OnBrowserContextInitializedInternal,
          base::Unretained(this), browser_context, std::move(reply)));
}

void WebRtcEventLogManager::PeerConnectionAdded(
    int render_process_id,
    int lid,
    base::OnceCallback<void(bool)> reply) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&WebRtcEventLogManager::PeerConnectionAddedInternal,
                     base::Unretained(this), render_process_id, lid,
                     std::move(reply)));
}

void WebRtcEventLogManager::PeerConnectionRemoved(
    int render_process_id,
    int lid,
    base::OnceCallback<void(bool)> reply) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&WebRtcEventLogManager::PeerConnectionRemovedInternal,
                     base::Unretained(this), render_process_id, lid,
                     GetBrowserContext(render_process_id), std::move(reply)));
}

void WebRtcEventLogManager::EnableLocalLogging(
    base::FilePath base_path,
    size_t max_file_size_bytes,
    base::OnceCallback<void(bool)> reply) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(!base_path.empty());
  task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&WebRtcEventLogManager::EnableLocalLoggingInternal,
                     base::Unretained(this), base_path, max_file_size_bytes,
                     std::move(reply)));
}

void WebRtcEventLogManager::DisableLocalLogging(
    base::OnceCallback<void(bool)> reply) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&WebRtcEventLogManager::DisableLocalLoggingInternal,
                     base::Unretained(this), std::move(reply)));
}

void WebRtcEventLogManager::StartRemoteLogging(
    int render_process_id,
    int lid,
    size_t max_file_size_bytes,
    base::OnceCallback<void(bool)> reply) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&WebRtcEventLogManager::StartRemoteLoggingInternal,
                     base::Unretained(this), render_process_id, lid,
                     GetBrowserContext(render_process_id), max_file_size_bytes,
                     std::move(reply)));
}

void WebRtcEventLogManager::OnWebRtcEventLogWrite(
    int render_process_id,
    int lid,
    const std::string& output,
    base::OnceCallback<void(bool)> reply) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&WebRtcEventLogManager::OnWebRtcEventLogWriteInternal,
                     base::Unretained(this), render_process_id, lid,
                     GetBrowserContext(render_process_id), output,
                     std::move(reply)));
}

void WebRtcEventLogManager::SetLocalLogsObserver(
    WebRtcLocalEventLogsObserver* observer,
    base::OnceClosure reply) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&WebRtcEventLogManager::SetLocalLogsObserverInternal,
                     base::Unretained(this), observer, std::move(reply)));
}

void WebRtcEventLogManager::SetRemoteLogsObserver(
    WebRtcRemoteEventLogsObserver* observer,
    base::OnceClosure reply) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&WebRtcEventLogManager::SetRemoteLogsObserverInternal,
                     base::Unretained(this), observer, std::move(reply)));
}

void WebRtcEventLogManager::SetTaskRunnerForTesting(
    const scoped_refptr<base::SequencedTaskRunner>& task_runner) {
  task_runner_ = task_runner;
}

scoped_refptr<base::SequencedTaskRunner>&
WebRtcEventLogManager::GetTaskRunnerForTesting() {
  return task_runner_;
}

void WebRtcEventLogManager::OnLocalLogStarted(PeerConnectionKey peer_connection,
                                              base::FilePath file_path) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());

  OnLoggingTargetStarted(LoggingTarget::kLocalLogging, peer_connection);

  if (local_logs_observer_) {
    local_logs_observer_->OnLocalLogStarted(peer_connection, file_path);
  }
}

void WebRtcEventLogManager::OnLocalLogStopped(
    PeerConnectionKey peer_connection) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());

  OnLoggingTargetStopped(LoggingTarget::kLocalLogging, peer_connection);

  if (local_logs_observer_) {
    local_logs_observer_->OnLocalLogStopped(peer_connection);
  }
}

void WebRtcEventLogManager::OnRemoteLogStarted(PeerConnectionKey key,
                                               base::FilePath file_path) {
  if (remote_logs_observer_) {
    remote_logs_observer_->OnRemoteLogStarted(key, file_path);
  }
}

void WebRtcEventLogManager::OnRemoteLogStopped(
    WebRtcEventLogPeerConnectionKey key) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  OnLoggingTargetStopped(LoggingTarget::kRemoteLogging, key);
  if (remote_logs_observer_) {
    remote_logs_observer_->OnRemoteLogStopped(key);
  }
}

void WebRtcEventLogManager::OnLoggingTargetStarted(LoggingTarget target,
                                                   PeerConnectionKey key) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  auto it = peer_connections_with_event_logging_enabled_.find(key);
  if (it != peer_connections_with_event_logging_enabled_.end()) {
    DCHECK_EQ((it->second & target), 0u);
    it->second |= target;
  } else {
    // This is the first client for WebRTC event logging - let WebRTC know
    // that it should start informing us of events.
    peer_connections_with_event_logging_enabled_.emplace(key, target);
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::BindOnce(&WebRtcEventLogManager::UpdateWebRtcEventLoggingState,
                       base::Unretained(this), key, true));
  }
}

void WebRtcEventLogManager::OnLoggingTargetStopped(LoggingTarget target,
                                                   PeerConnectionKey key) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());

  // Record that we're no longer performing this type of logging for this PC.
  auto it = peer_connections_with_event_logging_enabled_.find(key);
  CHECK(it != peer_connections_with_event_logging_enabled_.end());
  DCHECK_NE(it->second, 0u);
  it->second &= ~target;

  // If we're not doing any other type of logging for this peer connection,
  // it's time to stop receiving notifications for it from WebRTC.
  if (it->second == 0u) {
    peer_connections_with_event_logging_enabled_.erase(it);
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::BindOnce(&WebRtcEventLogManager::UpdateWebRtcEventLoggingState,
                       base::Unretained(this), key, false));
  }
}

void WebRtcEventLogManager::OnBrowserContextInitializedInternal(
    const BrowserContext* browser_context,
    base::OnceClosure reply) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  remote_logs_manager_.OnBrowserContextInitialized(browser_context);
  if (reply) {
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE, std::move(reply));
  }
}

void WebRtcEventLogManager::PeerConnectionAddedInternal(
    int render_process_id,
    int lid,
    base::OnceCallback<void(bool)> reply) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  const bool local_result =
      local_logs_manager_.PeerConnectionAdded(render_process_id, lid);
  const bool remote_result =
      remote_logs_manager_.PeerConnectionAdded(render_process_id, lid);
  DCHECK_EQ(local_result, remote_result);
  if (reply) {
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                            base::BindOnce(std::move(reply), local_result));
  }
}

void WebRtcEventLogManager::PeerConnectionRemovedInternal(
    int render_process_id,
    int lid,
    const BrowserContext* browser_context,
    base::OnceCallback<void(bool)> reply) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  const bool local_result =
      local_logs_manager_.PeerConnectionRemoved(render_process_id, lid);
  const bool remote_result = remote_logs_manager_.PeerConnectionRemoved(
      render_process_id, lid, browser_context);
  DCHECK_EQ(local_result, remote_result);
  if (reply) {
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                            base::BindOnce(std::move(reply), local_result));
  }
}

void WebRtcEventLogManager::EnableLocalLoggingInternal(
    base::FilePath base_path,
    size_t max_file_size_bytes,
    base::OnceCallback<void(bool)> reply) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  const bool result =
      local_logs_manager_.EnableLogging(base_path, max_file_size_bytes);
  if (reply) {
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                            base::BindOnce(std::move(reply), result));
  }
}

void WebRtcEventLogManager::DisableLocalLoggingInternal(
    base::OnceCallback<void(bool)> reply) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  const bool result = local_logs_manager_.DisableLogging();
  if (reply) {
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                            base::BindOnce(std::move(reply), result));
  }
}

void WebRtcEventLogManager::StartRemoteLoggingInternal(
    int render_process_id,
    int lid,
    const BrowserContext* browser_context,
    size_t max_file_size_bytes,
    base::OnceCallback<void(bool)> reply) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  DCHECK(browser_context);
  const bool result = remote_logs_manager_.StartRemoteLogging(
      render_process_id, lid, browser_context, max_file_size_bytes);
  if (result) {
    const PeerConnectionKey key(render_process_id, lid);
    OnLoggingTargetStarted(LoggingTarget::kRemoteLogging, key);
  }
  if (reply) {
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                            base::BindOnce(std::move(reply), result));
  }
}

void WebRtcEventLogManager::OnWebRtcEventLogWriteInternal(
    int render_process_id,
    int lid,
    const BrowserContext* browser_context,
    const std::string& output,
    base::OnceCallback<void(bool)> reply) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  const bool local_result =
      local_logs_manager_.EventLogWrite(render_process_id, lid, output);
  // TODO: !!! Unit test that shows that it's OK if these two give different
  // results.
  const bool remote_result =
      remote_logs_manager_.EventLogWrite(render_process_id, lid, output);
  if (reply) {
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        // TODO: !!! Is it really useful to return the AND of these? Maybe
        // we should return both instead?
        base::BindOnce(std::move(reply), local_result && remote_result));
  }
}

void WebRtcEventLogManager::SetLocalLogsObserverInternal(
    WebRtcLocalEventLogsObserver* observer,
    base::OnceClosure reply) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  local_logs_observer_ = observer;
  if (reply) {
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE, std::move(reply));
  }
}

void WebRtcEventLogManager::SetRemoteLogsObserverInternal(
    WebRtcRemoteEventLogsObserver* observer,
    base::OnceClosure reply) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  remote_logs_observer_ = observer;
  if (reply) {
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE, std::move(reply));
  }
}

void WebRtcEventLogManager::UpdateWebRtcEventLoggingState(
    PeerConnectionKey peer_connection,
    bool enabled) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (enabled) {
    pc_tracker_proxy_->StartEventLogOutput(peer_connection);
  } else {
    pc_tracker_proxy_->StopEventLogOutput(peer_connection);
  }
}

void WebRtcEventLogManager::SetClockForTesting(base::Clock* clock) {
  // Testing only; no need for threading guarantees (called before anything
  // relevant could be put on the TQ).
  local_logs_manager_.SetClockForTesting(clock);
}

void WebRtcEventLogManager::SetPeerConnectionTrackerProxyForTesting(
    std::unique_ptr<PeerConnectionTrackerProxy> pc_tracker_proxy) {
  // Testing only; no need for threading guarantees (called before anything
  // relevant could be put on the TQ).
  pc_tracker_proxy_ = std::move(pc_tracker_proxy);
}

}  // namespace content
