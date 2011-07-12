// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_FILTERS_CHUNK_DEMUXER_H_
#define MEDIA_FILTERS_CHUNK_DEMUXER_H_

#include <list>

#include "base/synchronization/lock.h"
#include "media/base/filters.h"
#include "media/webm/webm_cluster_parser.h"

struct AVFormatContext;

namespace media {

class ChunkDemuxerStream;

// Demuxer implementation that allows chunks of WebM media data to be passed
// from JavaScript to the media stack.
class ChunkDemuxer : public Demuxer {
 public:
  ChunkDemuxer();
  virtual ~ChunkDemuxer();

  void Init(PipelineStatusCB cb);

  // Filter implementation.
  virtual void set_host(FilterHost* filter_host);
  virtual void Stop(FilterCallback* callback);
  virtual void Seek(base::TimeDelta time, const FilterStatusCB&  cb);
  virtual void OnAudioRendererDisabled();
  virtual void SetPreload(Preload preload);

  // Demuxer implementation.
  virtual scoped_refptr<DemuxerStream> GetStream(DemuxerStream::Type type);
  virtual base::TimeDelta GetStartTime() const;

  // Methods used by MediaDataSink
  void FlushData();
  bool AppendData(const uint8* data, unsigned length);
  void Shutdown();

 private:
  enum State {
    WAITING_FOR_INIT,
    INITIALIZING,
    INITIALIZED,
    INIT_ERROR,
    SHUTDOWN,
  };

  void ChangeState(State new_state);

  // Parses a buffer that contains an INFO & TRACKS element. Returns false if
  // the parse fails. This method handles calling & clearing |init_cb_|
  // before it returns.
  bool ParseInfoAndTracks_Locked(const uint8* data, int size);

  // Generates an AVFormatContext for the INFO & TRACKS elements contained
  // in |data|. Returns NULL if parsing |data| fails.
  AVFormatContext* CreateFormatContext(const uint8* data, int size) const;

  // Sets up |audio_| & |video_| DemuxerStreams based on the data in
  // |format_context_|. Returns false if no valid audio or video stream were
  // found.
  bool SetupStreams();

  // Parse all the buffers in |pending_buffers_|. Returns false if parsing one
  // of the buffers fails.
  bool ParsePendingBuffers_Locked();

  // Parse a buffer that was passed to AppendData(). |data| is expected to
  // contain one or more WebM Clusters. Returns false if parsing the data fails.
  bool ParseAndAppendData_Locked(const uint8* data, int length);

  // Called when initialization fails. Handles calling & clearing init_cb_.
  void InitFailed_Locked();

  base::Lock lock_;
  State state_;

  PipelineStatusCB init_cb_;
  FilterStatusCB seek_cb_;

  scoped_refptr<ChunkDemuxerStream> audio_;
  scoped_refptr<ChunkDemuxerStream> video_;

  AVFormatContext* format_context_;

  int64 buffered_bytes_;

  base::TimeDelta duration_;

  scoped_ptr<WebMClusterParser> cluster_parser_;

  typedef std::list<scoped_refptr<media::Buffer> > BufferList;
  BufferList pending_buffers_;

  // Should a Seek() call wait for more data before calling the
  // callback.
  bool seek_waits_for_data_;

  DISALLOW_COPY_AND_ASSIGN(ChunkDemuxer);
};

}  // namespace media

#endif  // MEDIA_FILTERS_CHUNK_DEMUXER_H_
