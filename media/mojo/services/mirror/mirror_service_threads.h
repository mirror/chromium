// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_MIRROR_MIRROR_SERVICE_THREADS_H_
#define MEDIA_MOJO_SERVICES_MIRROR_MIRROR_SERVICE_THREADS_H_

#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread.h"

class MirrorServiceThreads {
 public:
  scoped_refptr<base::SingleThreadTaskRunner> GetAudioEncodeTaskRunner();
  scoped_refptr<base::SingleThreadTaskRunner> GetVideoEncodeTaskRunner();

 private:
  friend struct base::LazyInstanceTraitsBase<MirrorServiceThreads>;

  MirrorServiceThreads();

  base::Thread audio_encode_thread_;
  base::Thread video_encode_thread_;

  DISALLOW_COPY_AND_ASSIGN(MirrorServiceThreads);
};

#endif  // MEDIA_MOJO_SERVICES_MIRROR_MIRROR_SERVICE_THREADS_H_
