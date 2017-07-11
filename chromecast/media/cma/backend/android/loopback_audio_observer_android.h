// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include <vector>

#include "base/macros.h"
#include "base/threading/thread.h"
#include "chromecast/public/cast_media_shlib.h"

struct AI2sDevice;

namespace chromecast {
namespace media {

class LoopbackAudioManager {
 public:
  // Get singleton instance of Loopback Audio Manager
  static LoopbackAudioManager* Get();

  // Adds a loopback audio observer.
  void AddLoopbackAudioObserver(
      CastMediaShlib::LoopbackAudioObserver* observer);

  // Removes a loopback audio observer.
  void RemoveLoopbackAudioObserver(
      CastMediaShlib::LoopbackAudioObserver* observer);

 protected:
  LoopbackAudioManager();
  virtual ~LoopbackAudioManager();

 private:
  void StartLoopback();
  void StopLoopback();
  void RunLoopback();
  void CalibrateTimestamp(int64_t frame_position);
  int64_t GetInterpolatedTimestamp(int64_t frame_position);

  std::vector<CastMediaShlib::LoopbackAudioObserver*> loopback_observers_;

  AI2sDevice* i2s_;

  int64_t last_timestamp_nsec_;
  int64_t last_frame_position_;
  int64_t frame_count_;

  // Thread that feeds audio data into the observer from the loopback.  The
  // calls made on this thread are blocking.
  base::Thread feeder_thread_;
  bool loopback_running_;

  DISALLOW_COPY_AND_ASSIGN(LoopbackAudioManager);
};

}  // namespace media
}  // namespace chromecast
