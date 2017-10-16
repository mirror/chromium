// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_VIDEO_DECODE_PERF_HISTORY_H_
#define MEDIA_MOJO_SERVICES_VIDEO_DECODE_PERF_HISTORY_H_

#include <stdint.h>
#include <memory>
#include <string>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/sequence_checker.h"
#include "media/base/video_codecs.h"
#include "media/mojo/interfaces/video_decode_perf_history.mojom.h"
#include "media/mojo/services/media_mojo_export.h"
#include "media/mojo/services/video_decode_stats_db.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "ui/gfx/geometry/size.h"

namespace media {

// This browser-process service helps render-process clients respond to
// MediaCapabilities queries by looking up past performance for a given video
// configuration.
//
// Smoothness and power efficiency are assessed by evaluating raw stats from the
// MediaCapabilitiesDatabse.
//
// The object is lazily created upon the first call to BindRequest() (via Mojo)
// or GetSingletonInstance(). Future calls to BindRequest() will bind to the
// same instance.
//
// This class is not thread safe. All calls to BindRequest() and GetPerfInfo()
// should be made on the same sequence (generally stemming from inbound Mojo
// IPCs on the browser IO thread). ClearHistory() is special case that may be
// called from any sequence.
class MEDIA_MOJO_EXPORT VideoDecodePerfHistory
    : public mojom::VideoDecodePerfHistory {
 public:
  // Provides |db_instance| for use once VideoDecodePerfHistory is lazily (upon
  // first BindRequest) instantiated. Database lifetime should match/exceed that
  // of the VideoDecodePerfHistory singleton.
  static void Initialize(VideoDecodeStatsDB* db_instance);

  // Grab a pointer to the singleton instance (potentially lazily creating it).
  // Requires that Initialize has previously been called.
  static VideoDecodePerfHistory* GetInstance();

  // Bind the request to singleton instance.
  static void BindRequest(mojom::VideoDecodePerfHistoryRequest request);

  // mojom::VideoDecodePerfHistory implementation:
  void GetPerfInfo(VideoCodecProfile profile,
                   const gfx::Size& natural_size,
                   int frame_rate,
                   GetPerfInfoCallback callback) override;

  // Clears the history and runs |callback| when done. May be called from any
  // sequence. |callback| will be executed on the same sequence as the call.
  void ClearHistory(base::OnceClosure callback);

 private:
  // Friends so it can create its own instances with mock database.
  friend class VideoDecodePerfHistoryTest;

  // Decode capabilities will be described as "smooth" whenever the percentage
  // of dropped frames is less-than-or-equal-to this value. 10% chosen as a
  // lenient value after manual testing.
  static constexpr double kMaxSmoothDroppedFramesPercent = .10;

  VideoDecodePerfHistory();
  ~VideoDecodePerfHistory() override;

  // Binds |request| to this instance.
  void BindRequestInternal(mojom::VideoDecodePerfHistoryRequest request);

  // Internal callback for |database_| queries. Will assess performance history
  // from database and pass results on to |mojo_cb|.
  void OnGotStatsEntry(
      const VideoDecodeStatsDB::VideoDescKey& key,
      GetPerfInfoCallback mojo_cb,
      bool database_success,
      std::unique_ptr<VideoDecodeStatsDB::DecodeStatsEntry> entry);

  // Maps bindings from several render-processes to this single browser-process
  // service.
  mojo::BindingSet<mojom::VideoDecodePerfHistory> bindings_;

  // The task runner belonging to the sequence used to construct this instance.
  // Used by ClearHistory() to trampoline calls from other sequences.
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  // Ensures all access to class members come on the same sequence.
  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(VideoDecodePerfHistory);
};

}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_VIDEO_DECODE_PERF_HISTORY_H_
