// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/media/audio_output_stream_observer_impl.h"

#include "content/browser/media/audio_stream_monitor.h"
#include "content/public/browser/browser_thread.h"

namespace content {

AudioOutputStreamObserverImpl::AudioOutputStreamObserverImpl(
    int render_process_id,
    int render_frame_id,
    int stream_id)
    : render_process_id_(render_process_id),
      render_frame_id_(render_frame_id),
      stream_id_(stream_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
}

AudioOutputStreamObserverImpl::~AudioOutputStreamObserverImpl() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
}

void AudioOutputStreamObserverImpl::DidStartPlaying(bool is_audible) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  AudioStreamMonitor::StartMonitoringStream(
      render_process_id_, render_frame_id_, stream_id_, is_audible);
}
void AudioOutputStreamObserverImpl::DidStopPlaying() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  AudioStreamMonitor::StopMonitoringStream(render_process_id_, render_frame_id_,
                                           stream_id_);
}

void AudioOutputStreamObserverImpl::DidChangeAudibleState(bool is_audible) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  AudioStreamMonitor::UpdateStreamAudibleState(
      render_process_id_, render_frame_id_, stream_id_, is_audible);
}

}  // namespace content
