// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_ANDROID_MCVD_SURFACE_CHOOSER_H_
#define MEDIA_GPU_ANDROID_MCVD_SURFACE_CHOOSER_H_

#include "base/macros.h"
#include "base/sequence_checker.h"
#include "base/single_thread_task_runner.h"
#include "media/base/android_overlay_mojo_factory.h"
#include "media/base/overlay_info.h"
#include "media/gpu/android/android_video_surface_chooser.h"
#include "media/gpu/media_gpu_export.h"

namespace media {

// McvdSurfaceChooser coordinates activities related to choosing and
// creating output surfaces for MediaCodecVideoDecoder. It can be created, run
// and deleted on any single sequence. It wraps an AndroidVideoSurfaceChooser
// that it accesses and deletes on the GPU main thread.
// TODO(watk): Integrate promotion hints.
class MEDIA_GPU_EXPORT McvdSurfaceChooser {
 public:
  using SurfaceChosenCB =
      base::RepeatingCallback<void(std::unique_ptr<AndroidOverlay> overlay)>;

  // Note: |overlay_mojo_factory_cb| will be run on |gpu_task_runner| and
  // |request_overlay_info_cb| will be run on the calling sequence.
  McvdSurfaceChooser(
      std::unique_ptr<AndroidVideoSurfaceChooser> surface_chooser,
      scoped_refptr<base::SingleThreadTaskRunner> gpu_task_runner,
      AndroidOverlayMojoFactoryCB overlay_mojo_factory_cb,
      RequestOverlayInfoCB request_overlay_info_cb);
  ~McvdSurfaceChooser();

  // Initializes the surface chooser. |surface_chosen_cb| will be run on the
  // calling sequence at least one time after this is called (more than once if
  // the wrapped surface chooser supports dynamic surface switching).
  // See AndroidVideoSurfaceChooser::State for |is_secure| and |is_required|.
  void Initialize(bool is_secure,
                  bool is_required,
                  SurfaceChosenCB surface_chosen_cb);

 private:
  // Forwards |info| to |gpu_thread_impl_|.
  void OnOverlayInfoChanged(const OverlayInfo& info);

  // The GPU thread side of the implementation.
  class Impl;
  std::unique_ptr<Impl> gpu_thread_impl_;
  scoped_refptr<base::SingleThreadTaskRunner> gpu_task_runner_;
  RequestOverlayInfoCB request_overlay_info_cb_;

  SEQUENCE_CHECKER(sequence_checker_);
  base::WeakPtrFactory<McvdSurfaceChooser> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(McvdSurfaceChooser);
};

}  // namespace media

#endif  // MEDIA_GPU_ANDROID_MCVD_SURFACE_CHOOSER_H_
