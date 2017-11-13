// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webrtc/webrtc_eventlog_host.h"

#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_traits.h"
#include "base/threading/thread_restrictions.h"
#include "content/browser/webrtc/webrtc_event_log_manager.h"
#include "content/browser/webrtc/webrtc_internals.h"
#include "content/common/media/peer_connection_tracker_messages.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"

namespace content {

namespace {

// A lower maximum filesize is used on Android because storage space is
// more scarce on mobile. This upper limit applies to each peerconnection
// individually, so the total amount of used storage can be a multiple of
// this.
// TODO(eladalon): The cap on the size is identical to that set in
// content/renderer/media/peer_connection_tracker.cc. The version there is
// intended for removal in an upcoming CL; the version here is inteded to be
// moved.
#if defined(OS_ANDROID)
const int kMaxNumberLogFiles = 3;
constexpr int64_t kMaxLocalRtcEventLogFileSizeBytes = 10000000;
#else
const int kMaxNumberLogFiles = 5;
constexpr int64_t kMaxLocalRtcEventLogFileSizeBytes = 60000000;
#endif

}  // namespace

WebRTCEventLogHost::WebRTCEventLogHost(int render_process_id)
    : render_process_id_(render_process_id),
      rtc_event_logging_enabled_(false),
      weak_ptr_factory_(this) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  auto* webrtc_internals = WebRTCInternals::GetInstance();
  if (webrtc_internals->IsEventLogRecordingsEnabled())
    StartLocalRtcEventLogging(webrtc_internals->GetEventLogFilePath());
}

WebRTCEventLogHost::~WebRTCEventLogHost() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  for (int lid : active_peer_connection_local_ids_) {
    RtcEventLogRemoved(lid);
  }
}

void WebRTCEventLogHost::PeerConnectionAdded(int peer_connection_local_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (std::find(active_peer_connection_local_ids_.begin(),
                active_peer_connection_local_ids_.end(),
                peer_connection_local_id) ==
      active_peer_connection_local_ids_.end()) {
    active_peer_connection_local_ids_.push_back(peer_connection_local_id);
    if (rtc_event_logging_enabled_ &&
        ActivePeerConnectionsWithLogFiles().size() < kMaxNumberLogFiles) {
      StartEventLogForPeerConnection(peer_connection_local_id);
    }
  }
}

void WebRTCEventLogHost::PeerConnectionRemoved(int peer_connection_local_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  const auto lid = std::find(active_peer_connection_local_ids_.begin(),
                             active_peer_connection_local_ids_.end(),
                             peer_connection_local_id);
  if (lid != active_peer_connection_local_ids_.end()) {
    active_peer_connection_local_ids_.erase(lid);
  }
  RtcEventLogRemoved(peer_connection_local_id);
}

bool WebRTCEventLogHost::StartLocalRtcEventLogging(
    const base::FilePath& base_path) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (rtc_event_logging_enabled_)
    return false;
  rtc_event_logging_enabled_ = true;
  base_file_path_ = base_path;
  for (int local_id : active_peer_connection_local_ids_)
    StartEventLogForPeerConnection(local_id);
  return true;
}

bool WebRTCEventLogHost::StopLocalRtcEventLogging() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!rtc_event_logging_enabled_) {
    DCHECK(ActivePeerConnectionsWithLogFiles().empty());
    return false;
  }
  rtc_event_logging_enabled_ = false;
  RenderProcessHost* host = RenderProcessHost::FromID(render_process_id_);
  if (host) {
    for (int local_id : active_peer_connection_local_ids_) {
      host->Send(new PeerConnectionTracker_StopEventLog(local_id));
      RtcEventLogManager::GetInstance()->LocalRtcEventLogStop(
          render_process_id_, local_id);
    }
  }
  ActivePeerConnectionsWithLogFiles().clear();
  return true;
}

base::WeakPtr<WebRTCEventLogHost> WebRTCEventLogHost::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

std::vector<WebRTCEventLogHost::PeerConnectionKey>&
WebRTCEventLogHost::ActivePeerConnectionsWithLogFiles() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  CR_DEFINE_STATIC_LOCAL(std::vector<WebRTCEventLogHost::PeerConnectionKey>,
                         vector, ());
  return vector;
}

void WebRTCEventLogHost::StartEventLogForPeerConnection(
    int peer_connection_local_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  RenderProcessHost* rph = RenderProcessHost::FromID(render_process_id_);
  if (rph && ActivePeerConnectionsWithLogFiles().size() < kMaxNumberLogFiles) {
    RtcEventLogAdded(peer_connection_local_id);
    RtcEventLogManager::GetInstance()->LocalRtcEventLogStart(
        render_process_id_, peer_connection_local_id, base_file_path_,
        kMaxLocalRtcEventLogFileSizeBytes);
    rph->Send(new PeerConnectionTracker_StartEventLogOutput(
        peer_connection_local_id));
  }
}

void WebRTCEventLogHost::RtcEventLogAdded(int peer_connection_local_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  auto& active_peer_connections_with_log_files =
      ActivePeerConnectionsWithLogFiles();
  PeerConnectionKey pc_key = {render_process_id_, peer_connection_local_id};
  DCHECK(std::find(active_peer_connections_with_log_files.begin(),
                   active_peer_connections_with_log_files.end(),
                   pc_key) == active_peer_connections_with_log_files.end());
  active_peer_connections_with_log_files.push_back(pc_key);
}

bool WebRTCEventLogHost::RtcEventLogRemoved(int peer_connection_local_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  auto& active_peer_connections_with_log_files =
      ActivePeerConnectionsWithLogFiles();
  PeerConnectionKey pc_key = {render_process_id_, peer_connection_local_id};
  const auto it =
      std::find(active_peer_connections_with_log_files.begin(),
                active_peer_connections_with_log_files.end(), pc_key);
  if (it != active_peer_connections_with_log_files.end()) {
    active_peer_connections_with_log_files.erase(it);
    return true;
  }
  return false;
}

}  // namespace content
