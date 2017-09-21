// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/android/mcvd_surface_chooser.h"
#include "base/bind.h"
#include "media/base/bind_to_current_loop.h"

namespace media {

class MEDIA_GPU_EXPORT McvdSurfaceChooser::Impl {
 public:
  Impl(std::unique_ptr<AndroidVideoSurfaceChooser> surface_chooser,
       AndroidOverlayMojoFactoryCB overlay_mojo_factory_cb);
  ~Impl();

  void InitializeOnAnySequence(bool is_secure,
                               bool is_required,
                               SurfaceChosenCB surface_chosen_cb);

  // Notifies |surface_chooser_| of the new OverlayInfo.
  void OnOverlayInfoChanged(const OverlayInfo& info);

 private:
  std::unique_ptr<AndroidVideoSurfaceChooser> surface_chooser_;
  bool initialized_surface_chooser_ = false;

  OverlayInfo overlay_info_;
  AndroidVideoSurfaceChooser::State chooser_state_;

  SurfaceChosenCB surface_chosen_cb_;
  AndroidOverlayMojoFactoryCB overlay_mojo_factory_cb_;

  SEQUENCE_CHECKER(sequence_checker_);
  DISALLOW_COPY_AND_ASSIGN(Impl);
};

McvdSurfaceChooser::Impl::Impl(
    std::unique_ptr<AndroidVideoSurfaceChooser> surface_chooser,
    AndroidOverlayMojoFactoryCB overlay_mojo_factory_cb)
    : surface_chooser_(std::move(surface_chooser)),
      overlay_mojo_factory_cb_(std::move(overlay_mojo_factory_cb)) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

McvdSurfaceChooser::Impl::~Impl() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void McvdSurfaceChooser::Impl::InitializeOnAnySequence(
    bool is_secure,
    bool is_required,
    SurfaceChosenCB surface_chosen_cb) {
  chooser_state_.is_secure = is_secure;
  chooser_state_.is_required = is_required;
  surface_chosen_cb_ = std::move(surface_chosen_cb);
}

void McvdSurfaceChooser::Impl::OnOverlayInfoChanged(const OverlayInfo& info) {
  DVLOG(2) << __func__;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  bool overlay_changed = !overlay_info_.RefersToSameOverlayAs(info);
  overlay_info_ = info;

  AndroidOverlayFactoryCB factory_cb;
  if (overlay_info_.HasValidRoutingToken() && overlay_mojo_factory_cb_) {
    factory_cb =
        base::Bind(overlay_mojo_factory_cb_, *overlay_info_.routing_token);
  }

  // Call Initialize() the first time, and UpdateState() thereafter.
  if (!initialized_surface_chooser_) {
    initialized_surface_chooser_ = true;
    surface_chooser_->Initialize(surface_chosen_cb_,
                                 base::Bind(surface_chosen_cb_, nullptr),
                                 std::move(factory_cb), chooser_state_);
  } else {
    surface_chooser_->UpdateState(
        overlay_changed ? base::make_optional(std::move(factory_cb))
                        : base::nullopt,
        chooser_state_);
  }
}

McvdSurfaceChooser::McvdSurfaceChooser(
    std::unique_ptr<AndroidVideoSurfaceChooser> surface_chooser,
    scoped_refptr<base::SingleThreadTaskRunner> gpu_task_runner,
    AndroidOverlayMojoFactoryCB overlay_mojo_factory_cb,
    RequestOverlayInfoCB request_overlay_info_cb)
    : gpu_thread_impl_(
          base::MakeUnique<Impl>(std::move(surface_chooser),
                                 std::move(overlay_mojo_factory_cb))),
      gpu_task_runner_(std::move(gpu_task_runner)),
      request_overlay_info_cb_(std::move(request_overlay_info_cb)),
      weak_factory_(this) {
  DVLOG(2) << __func__;
}

McvdSurfaceChooser::~McvdSurfaceChooser() {
  DVLOG(2) << __func__;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  gpu_task_runner_->DeleteSoon(FROM_HERE, gpu_thread_impl_.release());
}

void McvdSurfaceChooser::Initialize(bool is_secure,
                                    bool is_required,
                                    SurfaceChosenCB surface_chosen_cb) {
  DVLOG(2) << __func__;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(request_overlay_info_cb_);

  // This is safe because we only call it once before posting any tasks.
  gpu_thread_impl_->InitializeOnAnySequence(
      is_secure, is_required, BindToCurrentLoop(surface_chosen_cb));

  // Request OverlayInfo updates.
  // TODO(watk): Set this value correctly.
  bool requires_restart_for_transitions = true;
  std::move(request_overlay_info_cb_)
      .Run(requires_restart_for_transitions,
           base::Bind(&McvdSurfaceChooser::OnOverlayInfoChanged,
                      weak_factory_.GetWeakPtr()));
  return;
}

void McvdSurfaceChooser::OnOverlayInfoChanged(const OverlayInfo& info) {
  DVLOG(2) << __func__;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  gpu_task_runner_->PostTask(
      FROM_HERE, base::Bind(&Impl::OnOverlayInfoChanged,
                            base::Unretained(gpu_thread_impl_.get()), info));
}

}  // namespace media
