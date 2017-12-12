// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webrtc/webrtc_event_log_manager.h"

// TODO: !!!
#include "base/task_scheduler/post_task.h"
#include "content/common/media/peer_connection_tracker_messages.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"

namespace content {

base::LazyInstance<WebRtcEventLogManager>::Leaky g_webrtc_event_log_manager =
    LAZY_INSTANCE_INITIALIZER;

WebRtcEventLogManager* WebRtcEventLogManager::GetInstance() {
  return g_webrtc_event_log_manager.Pointer();
}

WebRtcEventLogManager::WebRtcEventLogManager()
    : local_logs_observer_(nullptr),
      local_logs_handler_(this),
      task_runner_(base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND,
           base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN})) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
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

void WebRtcEventLogManager::OnWebRtcEventLogWrite(
    int render_process_id,
    int lid,
    const std::string& output,
    base::OnceCallback<void(bool)> reply) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&WebRtcEventLogManager::OnWebRtcEventLogWriteInternal,
                     base::Unretained(this), render_process_id, lid, output,
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

// TODO: !!! s/Logs/Log
void WebRtcEventLogManager::OnLocalLogsStarted(
    PeerConnectionKey peer_connection,
    base::FilePath file_path) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());

  auto it = peer_connections_with_event_logging_enabled_.find(peer_connection);
  if (it != peer_connections_with_event_logging_enabled_.end()) {
    // We're already receiving WebRTC event logs for this peer connection.
    // Keep track that we also need it for local logging, so that if all
    // other reasons (remote logging) stop holding, we still keep it on for
    // local logging.
    DCHECK_EQ((it->second & LoggingTarget::kLocalLogging), 0);
    it->second |= LoggingTarget::kLocalLogging;
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::BindOnce(&WebRtcEventLogManager::UpdateWebRtcEventLoggingState,
                       base::Unretained(this), peer_connection, true));
  } else {
    // This is the first client for WebRTC event logging - let WebRTC know
    // that it should start informing us of events.
    peer_connections_with_event_logging_enabled_.emplace(
        peer_connection, LoggingTarget::kLocalLogging);
  }

  local_logs_observer_->OnLocalLogsStarted(peer_connection, file_path);
}

// TODO: !!! s/Logs/Log
void WebRtcEventLogManager::OnLocalLogsStopped(
    PeerConnectionKey peer_connection) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());

  // Record that we're no longer performing local-logging for this PC.
  auto it = peer_connections_with_event_logging_enabled_.find(peer_connection);
  CHECK(it != peer_connections_with_event_logging_enabled_.end());
  DCHECK_NE((it->second & LoggingTarget::kLocalLogging), 0);
  it->second &= ~LoggingTarget::kLocalLogging;

  // If we're not doing any other type of logging for this peer connection,
  // it's time to stop receiving notifications for it from WebRTC.
  if (it->second == 0) {
    peer_connections_with_event_logging_enabled_.erase(it);
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::BindOnce(&WebRtcEventLogManager::UpdateWebRtcEventLoggingState,
                       base::Unretained(this), peer_connection, false));
  }

  local_logs_observer_->OnLocalLogsStopped(peer_connection);
}

void WebRtcEventLogManager::PeerConnectionAddedInternal(
    int render_process_id,
    int lid,
    base::OnceCallback<void(bool)> reply) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  const bool result =
      local_logs_handler_.PeerConnectionAdded(render_process_id, lid);
  if (reply) {
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                            base::BindOnce(std::move(reply), result));
  }
}

void WebRtcEventLogManager::PeerConnectionRemovedInternal(
    int render_process_id,
    int lid,
    base::OnceCallback<void(bool)> reply) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  const bool result =
      local_logs_handler_.PeerConnectionRemoved(render_process_id, lid);
  if (reply) {
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                            base::BindOnce(std::move(reply), result));
  }
}

void WebRtcEventLogManager::EnableLocalLoggingInternal(
    base::FilePath base_path,
    size_t max_file_size_bytes,
    base::OnceCallback<void(bool)> reply) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  const bool result =
      local_logs_handler_.EnableLocalLogging(base_path, max_file_size_bytes);
  if (reply) {
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                            base::BindOnce(std::move(reply), result));
  }
}

void WebRtcEventLogManager::DisableLocalLoggingInternal(
    base::OnceCallback<void(bool)> reply) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  const bool result = local_logs_handler_.DisableLocalLogging();
  if (reply) {
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                            base::BindOnce(std::move(reply), result));
  }
}

void WebRtcEventLogManager::OnWebRtcEventLogWriteInternal(
    int render_process_id,
    int lid,  // Renderer-local PeerConnection ID.
    const std::string& output,
    base::OnceCallback<void(bool)> reply) {
  DCHECK(task_runner_->RunsTasksInCurrentSequence());
  const bool result =
      local_logs_handler_.WriteToLocalLogFile(render_process_id, lid, output);
  if (reply) {
    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                            base::BindOnce(std::move(reply), result));
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
  // TODO: !!! Unit test.
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

void WebRtcEventLogManager::InjectClockForTesting(base::Clock* clock) {
  // Testing only; no need for threading guarantees (called before anything
  // could be put on the TQ).
  local_logs_handler_.InjectClockForTesting(clock);
}

}  // namespace content
