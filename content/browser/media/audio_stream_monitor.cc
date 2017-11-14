// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/audio_stream_monitor.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/invalidate_type.h"

namespace content {

namespace {

AudioStreamMonitor* StartStopMonitoringHelper(int render_process_id,
                                              int render_frame_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // It's important that this code uses only the process id for lookup as there
  // may not be a RenderFrameHost or WebContents attached to the RenderProcess
  // at time of call; e.g., in the event of a crash.
  RenderProcessHost* const render_process_host =
      RenderProcessHost::FromID(render_process_id);
  if (!render_process_host)
    return nullptr;

  WebContentsImpl* const web_contents =
      static_cast<WebContentsImpl*>(WebContents::FromRenderFrameHost(
          RenderFrameHost::FromID(render_process_id, render_frame_id)));
  return web_contents ? web_contents->audio_stream_monitor() : nullptr;
}

}  // namespace

bool AudioStreamMonitor::StreamID::operator<(const StreamID& other) const {
  return std::tie(render_process_id, render_frame_id, stream_id) <
         std::tie(other.render_process_id, other.render_frame_id,
                  other.stream_id);
}

bool AudioStreamMonitor::StreamID::operator==(const StreamID& other) const {
  return std::tie(render_process_id, render_frame_id, stream_id) ==
         std::tie(other.render_process_id, other.render_frame_id,
                  other.stream_id);
}

AudioStreamMonitor::AudioStreamMonitor(WebContents* contents)
    : web_contents_(contents),
      clock_(&default_tick_clock_),
      indicator_is_on_(false),
      is_audible_(false) {
  DCHECK(web_contents_);
}

AudioStreamMonitor::~AudioStreamMonitor() {}

bool AudioStreamMonitor::WasRecentlyAudible() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return indicator_is_on_;
}

bool AudioStreamMonitor::IsCurrentlyAudible() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return is_audible_;
}

void AudioStreamMonitor::RenderProcessGone(int render_process_id) {
  DCHECK(thread_checker_.CalledOnValidThread());

  // Note: It's possible for the RenderProcessHost and WebContents (and thus
  // this class) to survive the death of the render process and subsequently be
  // reused. During this period StartStopMonitoringHelper() will be unable to
  // lookup the WebContents using the now-dead |render_frame_id|. We must thus
  // have this secondary mechanism for clearing stale callbacks.
  for (auto it = streams_.begin(); it != streams_.end();) {
    if (it->first.render_process_id == render_process_id) {
      it = streams_.erase(it);
      OnStreamRemoved();
    } else {
      ++it;
    }
  }
}

// static
void AudioStreamMonitor::StartMonitoringStream(int render_process_id,
                                               int render_frame_id,
                                               int stream_id) {
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&StartMonitoringHelper, render_process_id, render_frame_id,
                     stream_id));
}

// static
void AudioStreamMonitor::StopMonitoringStream(int render_process_id,
                                              int render_frame_id,
                                              int stream_id) {
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&StopMonitoringHelper, render_process_id, render_frame_id,
                     stream_id));
}

// static
void AudioStreamMonitor::UpdateStreamAudibleState(int render_process_id,
                                                  int render_frame_id,
                                                  int stream_id,
                                                  bool is_audible) {
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&UpdateStreamAudibleStateHelper, render_process_id,
                     render_frame_id, stream_id, is_audible));
}

// static
void AudioStreamMonitor::StartMonitoringHelper(int render_process_id,
                                               int render_frame_id,
                                               int stream_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (AudioStreamMonitor* monitor =
          StartStopMonitoringHelper(render_process_id, render_frame_id)) {
    monitor->StartMonitoringStreamOnUIThread(render_process_id, render_frame_id,
                                             stream_id);
  }
}

// static
void AudioStreamMonitor::StopMonitoringHelper(int render_process_id,
                                              int render_frame_id,
                                              int stream_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (AudioStreamMonitor* monitor =
          StartStopMonitoringHelper(render_process_id, render_frame_id)) {
    monitor->StopMonitoringStreamOnUIThread(render_process_id, render_frame_id,
                                            stream_id);
  }
}

// static
void AudioStreamMonitor::UpdateStreamAudibleStateHelper(int render_process_id,
                                                        int render_frame_id,
                                                        int stream_id,
                                                        bool is_audible) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (AudioStreamMonitor* monitor =
          StartStopMonitoringHelper(render_process_id, render_frame_id)) {
    monitor->UpdateStreamAudibleStateOnUIThread(
        render_process_id, render_frame_id, stream_id, is_audible);
  }
}

void AudioStreamMonitor::StartMonitoringStreamOnUIThread(int render_process_id,
                                                         int render_frame_id,
                                                         int stream_id) {
  DCHECK(thread_checker_.CalledOnValidThread());

  const StreamID qualified_id = {render_process_id, render_frame_id, stream_id};
  DCHECK(streams_.find(qualified_id) == streams_.end());

  streams_[qualified_id] = false;

  // Sends audible signal to RenderFrameHost when there is no power level
  // monitoring, otherwise sends the signal when the stream becomes audible.
  if (!audible_state_monitoring_available()) {
    if (auto* render_frame_host = static_cast<RenderFrameHostImpl*>(
            RenderFrameHost::FromID(render_process_id, render_frame_id))) {
      render_frame_host->OnAudibleStateChanged(true);
    }
  }

  OnStreamAdded();
}

void AudioStreamMonitor::StopMonitoringStreamOnUIThread(int render_process_id,
                                                        int render_frame_id,
                                                        int stream_id) {
  DCHECK(thread_checker_.CalledOnValidThread());

  // In the event of render process death, these may have already been cleared.
  auto it =
      streams_.find(StreamID{render_process_id, render_frame_id, stream_id});
  if (it == streams_.end())
    return;

  streams_.erase(it);

  // Sends non-audible signal to RenderFrameHost when there is no power level
  // monitoring, otherwise sends the signal when the stream becomes non-audible.
  if (!audible_state_monitoring_available()) {
    if (auto* render_frame_host = static_cast<RenderFrameHostImpl*>(
            RenderFrameHost::FromID(render_process_id, render_frame_id))) {
      render_frame_host->OnAudibleStateChanged(false);
    }
  }

  OnStreamRemoved();
}

void AudioStreamMonitor::UpdateStreamAudibleStateOnUIThread(
    int render_process_id,
    int render_frame_id,
    int stream_id,
    bool is_audible) {
  DCHECK(thread_checker_.CalledOnValidThread());

  const StreamID qualified_id = {render_process_id, render_frame_id, stream_id};
  auto it = streams_.find(qualified_id);
  DCHECK(it != streams_.end());
  it->second = is_audible;
  UpdateStreams();
}

void AudioStreamMonitor::UpdateStreams() {
  bool was_audible = is_audible_;
  is_audible_ = false;

  // Record whether or not a RenderFrameHost is audible.
  base::flat_map<RenderFrameHostImpl*, bool> audible_frame_map;
  audible_frame_map.reserve(streams_.size());
  for (auto& kv : streams_) {
    const bool is_stream_audible = kv.second;
    if (is_stream_audible)
      is_audible_ = true;

    // Record whether or not the RenderFrame is audible. A RenderFrame is
    // audible when it has at least one audio stream that is audible.
    auto* render_frame_host_impl =
        static_cast<RenderFrameHostImpl*>(RenderFrameHost::FromID(
            kv.first.render_process_id, kv.first.render_frame_id));
    // This may be nullptr in tests.
    if (!render_frame_host_impl)
      continue;
    audible_frame_map[render_frame_host_impl] |= is_stream_audible;
  }

  if (!was_audible && is_audible_)
    last_became_audible_time_ = clock_->NowTicks();

  // Update RenderFrameHost audible state only when state changed.
  for (auto& kv : audible_frame_map) {
    auto* render_frame_host_impl = kv.first;
    bool is_frame_audible = kv.second;
    if (is_frame_audible != render_frame_host_impl->is_audible())
      render_frame_host_impl->OnAudibleStateChanged(is_frame_audible);
  }

  if (is_audible_ != was_audible) {
    MaybeToggle();
    web_contents_->OnAudioStateChanged(is_audible_);
  }
}

void AudioStreamMonitor::MaybeToggle() {
  const base::TimeTicks off_time =
      last_became_audible_time_ +
      base::TimeDelta::FromMilliseconds(kHoldOnMilliseconds);
  const base::TimeTicks now = clock_->NowTicks();
  const bool should_stop_timer = now >= off_time;
  const bool should_indicator_be_on = is_audible_ || !should_stop_timer;

  if (should_indicator_be_on != indicator_is_on_) {
    indicator_is_on_ = should_indicator_be_on;
    web_contents_->NotifyNavigationStateChanged(INVALIDATE_TYPE_TAB);
  }

  if (should_stop_timer) {
    off_timer_.Stop();
  } else if (!off_timer_.IsRunning()) {
    off_timer_.Start(
        FROM_HERE,
        off_time - now,
        base::Bind(&AudioStreamMonitor::MaybeToggle, base::Unretained(this)));
  }
}

void AudioStreamMonitor::OnStreamAdded() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (streams_.size() != 1u)
    return;

  if (!audible_state_monitoring_available()) {
    is_audible_ = true;
    web_contents_->OnAudioStateChanged(true);
    MaybeToggle();
  } else {
    UpdateStreams();
  }
}

void AudioStreamMonitor::OnStreamRemoved() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!streams_.empty())
    return;

  if (!audible_state_monitoring_available()) {
    is_audible_ = false;
    web_contents_->OnAudioStateChanged(false);
    MaybeToggle();
  } else {
    UpdateStreams();
  }
}

}  // namespace content
