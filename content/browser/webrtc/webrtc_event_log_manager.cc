// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webrtc/webrtc_event_log_manager.h"

#include "base/task_scheduler/post_task.h"
#include "content/common/media/peer_connection_tracker_messages.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"

namespace content {

const size_t kWebRtcEventLogManagerUnlimitedFileSize = 0;

base::LazyInstance<WebRtcEventLogManager>::Leaky g_webrtc_event_log_manager =
    LAZY_INSTANCE_INITIALIZER;

WebRtcEventLogManager* WebRtcEventLogManager::GetInstance() {
  return g_webrtc_event_log_manager.Pointer();
}

WebRtcEventLogManager::WebRtcEventLogManager()
    : local_logs_observer_(nullptr),
      local_logs_manager_(this),
      task_runner_(base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND,
           base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN})) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // TODO: !!!!!! There is a big problem with unit tests, because we don't
  // know when this will be completed, and can't block on this, which is flaky.
//  task_runner_->PostTask(
//      FROM_HERE, base::BindOnce(&WebRtcRemoteEventLogManager::Init,
//                                base::Unretained(&remote_logs_manager_)));
}

WebRtcEventLogManager::~WebRtcEventLogManager() {
  // This should never actually run, except in unit tests.
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
                     GetRemoteEventLogManager(render_process_id),
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
                     GetRemoteEventLogManager(render_process_id),
                     std::move(reply)));
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
                     GetRemoteEventLogManager(render_process_id),
                     max_file_size_bytes, std::move(reply)));
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
                     GetRemoteEventLogManager(render_process_id),
                     output, std::move(reply)));
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

void WebRtcEventLogManager::SetTaskRunnerForTesting(
    const scoped_refptr<base::SequencedTaskRunner>& task_runner) {
  task_runner_ = task_runner;
}

void WebRtcEventLogManager::OnLocalLogStarted(PeerConnectionKey peer_connection,
                                              base::FilePath file_path) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());

  auto it = peer_connections_with_event_logging_enabled_.find(peer_connection);
  if (it != peer_connections_with_event_logging_enabled_.end()) {
    // We're already receiving WebRTC event logs for this peer connection.
    // Keep track that we also need it for local logging, so that if all
    // other reasons (remote logging) stop holding, we still keep it on for
    // local logging.
    DCHECK_EQ((it->second & LoggingTarget::kLocalLogging), 0u);
    it->second |= LoggingTarget::kLocalLogging;
  } else {
    // This is the first client for WebRTC event logging - let WebRTC know
    // that it should start informing us of events.
    peer_connections_with_event_logging_enabled_.emplace(
        peer_connection, LoggingTarget::kLocalLogging);
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::BindOnce(&WebRtcEventLogManager::UpdateWebRtcEventLoggingState,
                       base::Unretained(this), peer_connection, true));
  }

  if (local_logs_observer_) {
    local_logs_observer_->OnLocalLogStarted(peer_connection, file_path);
  }
}

void WebRtcEventLogManager::OnLocalLogStopped(
    PeerConnectionKey peer_connection) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());

  // Record that we're no longer performing local-logging for this PC.
  auto it = peer_connections_with_event_logging_enabled_.find(peer_connection);
  CHECK(it != peer_connections_with_event_logging_enabled_.end());
  DCHECK_NE((it->second & LoggingTarget::kLocalLogging), 0u);
  it->second &= ~LoggingTarget::kLocalLogging;

  // If we're not doing any other type of logging for this peer connection,
  // it's time to stop receiving notifications for it from WebRTC.
  if (it->second == 0u) {
    peer_connections_with_event_logging_enabled_.erase(it);
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::BindOnce(&WebRtcEventLogManager::UpdateWebRtcEventLoggingState,
                       base::Unretained(this), peer_connection, false));
  }

  if (local_logs_observer_) {
    local_logs_observer_->OnLocalLogStopped(peer_connection);
  }
}

void WebRtcEventLogManager::PeerConnectionAddedInternal(
    int render_process_id,
    int lid,
    WebRtcRemoteEventLogManager* remote_manager,
    base::OnceCallback<void(bool)> reply) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());

  const bool local_result =
      local_logs_manager_.PeerConnectionAdded(render_process_id, lid);

  if (remote_manager) {
    const bool remote_result =
        remote_manager->PeerConnectionAdded(render_process_id, lid);
    DCHECK_EQ(local_result, remote_result);
  }

  if (reply) {
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                            base::BindOnce(std::move(reply), local_result));
  }
}

void WebRtcEventLogManager::PeerConnectionRemovedInternal(
    int render_process_id,
    int lid,
    WebRtcRemoteEventLogManager* remote_manager,
    base::OnceCallback<void(bool)> reply) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());

  const bool local_result =
      local_logs_manager_.PeerConnectionRemoved(render_process_id, lid);

  if (remote_manager) {
    const bool remote_result =
        remote_manager->PeerConnectionRemoved(render_process_id, lid);
    DCHECK_EQ(local_result, remote_result);
  }

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
    WebRtcRemoteEventLogManager* remote_manager,
    size_t max_file_size_bytes,
    base::OnceCallback<void(bool)> reply) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  DCHECK(remote_manager);

  const bool result = remote_manager->StartRemoteLogging(
      render_process_id, lid,
      max_file_size_bytes);

  if (reply) {
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                            base::BindOnce(std::move(reply), result));
  }
}

void WebRtcEventLogManager::OnWebRtcEventLogWriteInternal(
    int render_process_id,
    int lid,
    WebRtcRemoteEventLogManager* remote_manager,
    const std::string& output,
    base::OnceCallback<void(bool)> reply) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());

  const bool local_result =
      local_logs_manager_.EventLogWrite(render_process_id, lid, output);

  const bool remote_result =
      remote_manager ?
      remote_manager->PeerConnectionAdded(render_process_id, lid) : true;

  if (reply) {
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                            base::BindOnce(std::move(reply),
                            local_result && remote_result));
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

void WebRtcEventLogManager::UpdateWebRtcEventLoggingState(
    PeerConnectionKey peer_connection,
    bool enabled) {
  // TODO(eladalon): Add unit tests that would make sure that we really
  // instruct WebRTC to start/stop event logs. https://crbug.com/775415
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  RenderProcessHost* host =
      RenderProcessHost::FromID(peer_connection.render_process_id);
  if (!host) {
    return;  // The host has been asynchronously removed; not a problem.
  }
  if (enabled) {
    host->Send(
        new PeerConnectionTracker_StartEventLogOutput(peer_connection.lid));
  } else {
    host->Send(new PeerConnectionTracker_StopEventLog(peer_connection.lid));
  }
}

const BrowserContext* WebRtcEventLogManager::GetBrowserContext(
    int render_process_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // Search for pre-recorded mapping.
  const auto it =
      render_process_id_to_browser_context_mapping_.find(render_process_id);
  if (it != render_process_id_to_browser_context_mapping_.end()) {
    return it->second;
  }

  // Attempt to discover new mapping.
  RenderProcessHost* host = RenderProcessHost::FromID(render_process_id);
  if (!host) {
    return nullptr;  // The host has been asynchronously removed; not a problem.
  }
  const BrowserContext* const browser_context = host->GetBrowserContext();
  render_process_id_to_browser_context_mapping_.emplace(render_process_id,
                                                        browser_context);

  return browser_context;
}

WebRtcRemoteEventLogManager* WebRtcEventLogManager::GetRemoteEventLogManager(
    const BrowserContext* browser_context) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (!browser_context) {
    return nullptr;
  }

  auto it = remote_logs_managers_.find(browser_context);
  if (it != remote_logs_managers_.end()) {
    return &it->second;
  }

  // The first argument to emplace() is the key, and the second is an argument
  // to WebRtcRemoteEventLogManager's ctor.
  auto emp_it = remote_logs_managers_.emplace(browser_context, browser_context);
  DCHECK(emp_it.second);
  return &emp_it.first->second;
}

WebRtcRemoteEventLogManager* WebRtcEventLogManager::GetRemoteEventLogManager(
    int render_process_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  const BrowserContext* const browser_context =
      GetBrowserContext(render_process_id);
  return GetRemoteEventLogManager(browser_context);
}

void WebRtcEventLogManager::InjectClockForTesting(base::Clock* clock) {
  // Testing only; no need for threading guarantees (called before anything
  // could be put on the TQ).
  local_logs_manager_.InjectClockForTesting(clock);
}

}  // namespace content
