// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_VIDEO_DECODE_PERF_HISTORY_H_
#define MEDIA_MOJO_SERVICES_VIDEO_DECODE_PERF_HISTORY_H_

#include <stdint.h>
#include <memory>
#include <string>

#include "base/callback.h"
#include "base/sequence_checker.h"
#include "media/base/video_codecs.h"
#include "media/mojo/interfaces/video_decode_perf_history.mojom.h"
#include "media/mojo/services/media_capabilities_database.h"
#include "media/mojo/services/media_mojo_export.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "ui/gfx/geometry/size.h"

namespace media {

// FIXME better doc
// Singleton object that lives in the browser process. See
// mojom::VideoDecodePerfHistory for documentation.
class MEDIA_MOJO_EXPORT VideoDecodePerfHistory
    : public mojom::VideoDecodePerfHistory {
 public:
  // Tests may call this to inject a mock database. The callback will be called
  // during construction of the VideoDecodePerfHistory singleton (lazily upon
  // the first call to BindRequest()).
  static void SetDatabaseProviderCallback_ForTesting(
      base::OnceCallback<std::unique_ptr<MediaCapabilitiesDatabase>()>
          callback);

  // Bind the request to singleton instance
  static void BindRequest(mojom::VideoDecodePerfHistoryRequest request);

  // mojom::VideoDecodePerfHistory implementation:
  // FIXME use VideoConfig from decode_capabilities.h
  void GetPerfInfo(VideoCodecProfile profile,
                   const gfx::Size& natural_size,
                   int frame_rate,
                   GetPerfInfoCallback callback) override;

 private:
  VideoDecodePerfHistory();
  ~VideoDecodePerfHistory() override;

  // Binds |request| to this instance.
  void BindRequestInternal(mojom::VideoDecodePerfHistoryRequest request);

  // Internal callback for |database_| queries. Will assess performance history
  // from database and pass results on to |mojo_cb|.
  void OnGotPerfInfo(MediaCapabilitiesDatabase::Entry entry,
                     GetPerfInfoCallback mojo_cb,
                     bool database_success,
                     std::unique_ptr<MediaCapabilitiesDatabase::Info> info);

  // Maps bindings from several render-processes to this single browser-process
  // service.
  mojo::BindingSet<mojom::VideoDecodePerfHistory> bindings_;

  // Database interface to manage reading/writing of capabilities stats.
  std::unique_ptr<MediaCapabilitiesDatabase> database_;

  // Ensures all access to class members come on the same sequence.
  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(VideoDecodePerfHistory);
};

}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_VIDEO_DECODE_PERF_HISTORY_H_