// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/android/android_video_surface_chooser_impl.h"

#include "base/memory/ptr_util.h"
#include "base/time/default_tick_clock.h"

namespace media {

// Minimum time that we require after a failed overlay attempt before we'll try
// again for an overlay.
constexpr base::TimeDelta MinimumDelayAfterFailedOverlay =
    base::TimeDelta::FromSeconds(5);

AndroidVideoSurfaceChooserImpl::AndroidVideoSurfaceChooserImpl(
    bool allow_dynamic,
    const AndroidOverlayMojoFactoryCB& overlay_mojo_factory_cb,
    base::TickClock* tick_clock)
    : overlay_mojo_factory_cb_(std::move(overlay_mojo_factory_cb)),
      allow_dynamic_(allow_dynamic),
      tick_clock_(tick_clock),
      weak_factory_(this) {
  // Use a DefaultTickClock if one wasn't provided.
  if (!tick_clock_) {
    optional_tick_clock_ = base::MakeUnique<base::DefaultTickClock>();
    tick_clock_ = optional_tick_clock_.get();
  }
}

AndroidVideoSurfaceChooserImpl::~AndroidVideoSurfaceChooserImpl() {}

void AndroidVideoSurfaceChooserImpl::Initialize(
    UseOverlayCB use_overlay_cb,
    UseSurfaceTextureCB use_surface_texture_cb) {
  use_overlay_cb_ = std::move(use_overlay_cb);
  use_surface_texture_cb_ = std::move(use_surface_texture_cb);
}

void AndroidVideoSurfaceChooserImpl::UpdateState(const State& state,
                                                 OverlayInfo overlay_info) {
  DCHECK(allow_dynamic_ || !first_update_received_);
  first_update_received_ = true;
  current_state_ = state;
  bool overlay_changed = !overlay_info.RefersToSameOverlayAs(overlay_info_);
  overlay_info_ = std::move(overlay_info);

  if (!allow_dynamic_) {
    if (overlay_info_.HasValidOverlay() &&
        (overlay_info_.is_fullscreen || state.is_secure || state.is_required)) {
      SwitchToOverlay();
    } else {
      SwitchToSurfaceTexture();
    }
    return;
  }

  if (overlay_changed) {
    // Cancel the previous overlay if we had one.
    overlay_ = nullptr;

    // The client's overlay is now the wrong one so clear the client state back
    // to kUnknown.
    if (client_overlay_state_ == kUsingOverlay)
      client_overlay_state_ = kUnknown;
  }

  Choose();
}

void AndroidVideoSurfaceChooserImpl::Choose() {
  // Pre-M, we decide on the first update.
  DCHECK(allow_dynamic_);

  OverlayState new_overlay_state = kUsingSurfaceTexture;

  // In player element fullscreen, we want to use overlays if we can.
  if (overlay_info_.is_fullscreen)
    new_overlay_state = kUsingOverlay;

  // Try to use an overlay if possible for protected content.  If the
  // compositor won't promote, though, it's okay if we switch out.  Set
  // |is_required| in addition, if you don't want this behavior.
  if (current_state_.is_secure)
    new_overlay_state = kUsingOverlay;

  // If the compositor won't promote, then don't.
  if (!current_state_.is_compositor_promotable)
    new_overlay_state = kUsingSurfaceTexture;

  // If we're expecting a relayout, then don't transition to overlay if we're
  // not already in one.  We don't want to transition out, though.  This lets
  // us delay entering on a fullscreen transition until blink relayout is
  // complete.
  // TODO(liberato): Detect this more directly.
  if (current_state_.is_expecting_relayout &&
      client_overlay_state_ != kUsingOverlay)
    new_overlay_state = kUsingSurfaceTexture;

  // If we're requesting an overlay, check that we haven't asked too recently
  // since the last failure.  This includes L1.  We don't bother to check for
  // our current state, since using an overlay would imply that our most
  // recent failure was long ago enough to pass this check earlier.
  if (new_overlay_state == kUsingOverlay) {
    base::TimeDelta time_since_last_failure =
        tick_clock_->NowTicks() - most_recent_overlay_failure_;
    if (time_since_last_failure < MinimumDelayAfterFailedOverlay)
      new_overlay_state = kUsingSurfaceTexture;
  }

  // If our frame is hidden, then don't use overlays.
  if (overlay_info_.is_frame_hidden)
    new_overlay_state = kUsingSurfaceTexture;

  // If an overlay is required, then choose one.  The only way we won't is if
  // we don't have a factory or our request fails.
  if (current_state_.is_required)
    new_overlay_state = kUsingOverlay;

  if (!overlay_info_.HasValidOverlay())
    new_overlay_state = kUsingSurfaceTexture;

  if (new_overlay_state == kUsingSurfaceTexture)
    SwitchToSurfaceTexture();
  else
    SwitchToOverlay();
}

void AndroidVideoSurfaceChooserImpl::SwitchToSurfaceTexture() {
  // Invalidate any outstanding deletion callbacks for any overlays that we've
  // provided to the client already.  We assume that it will eventually drop
  // them in response to the callback.  Ready / failed callbacks aren't
  // affected by this, since we own the overlay until those occur.  We're
  // about to drop |overlay_|, if we have one, which cancels them.
  weak_factory_.InvalidateWeakPtrs();

  // Cancel any outstanding overlay request.
  overlay_ = nullptr;

  // Notify the client to switch if it's in the wrong state.
  if (client_overlay_state_ != kUsingSurfaceTexture) {
    client_overlay_state_ = kUsingSurfaceTexture;
    use_surface_texture_cb_.Run();
  }
}

void AndroidVideoSurfaceChooserImpl::SwitchToOverlay() {
  // If there's already an overlay request outstanding, then do nothing. We'll
  // finish switching when it completes.
  if (overlay_)
    return;

  // Do nothing if the client is already using an overlay.
  if (client_overlay_state_ == kUsingOverlay)
    return;

  // We don't modify |client_overlay_state_| yet, since we don't call the
  // client back yet.

  // Invalidate any outstanding callbacks.  This is needed only for the
  // deletion callback, since for ready/failed callbacks, we still have
  // ownership of the object.  If we delete the object, then callbacks are
  // cancelled anyway.
  weak_factory_.InvalidateWeakPtrs();

  AndroidOverlayConfig config;
  // We bind all of our callbacks with weak ptrs, since we don't know how long
  // the client will hold on to overlays.  They could, in principle, show up
  // long after the client is destroyed too, if codec destruction hangs.
  config.ready_cb = base::Bind(&AndroidVideoSurfaceChooserImpl::OnOverlayReady,
                               weak_factory_.GetWeakPtr());
  config.failed_cb =
      base::Bind(&AndroidVideoSurfaceChooserImpl::OnOverlayFailed,
                 weak_factory_.GetWeakPtr());
  config.rect = current_state_.initial_position;
  overlay_ = overlay_info.HasValidRoutingToken()
                 ? overlay_mojo_factory_cb.Run(overlay_info.routing_token,
                                               std::move(config))
                 : std::make_unique<ContentVideoViewOverlay>(
                       overlay_info.surface_id, std::move(config));
  if (!overlay_)
    SwitchToSurfaceTexture();

  // We could add a destruction callback here, if we need to find out when the
  // surface has been destroyed.  It might also be good to have a 'overlay has
  // been destroyed' callback from ~AndroidOverlay, since we don't really know
  // how long that will take after SurfaceDestroyed.
}

void AndroidVideoSurfaceChooserImpl::OnOverlayReady(AndroidOverlay* overlay) {
  // |overlay_| is the only overlay for which we haven't gotten a ready
  // callback back yet.
  DCHECK_EQ(overlay, overlay_.get());

  // Notify the overlay that we'd like to know if it's destroyed, so that we
  // can update our internal state if the client drops it without being told.
  overlay_->AddOverlayDeletedCallback(
      base::Bind(&AndroidVideoSurfaceChooserImpl::OnOverlayDeleted,
                 weak_factory_.GetWeakPtr()));

  client_overlay_state_ = kUsingOverlay;
  use_overlay_cb_.Run(std::move(overlay_));
}

void AndroidVideoSurfaceChooserImpl::OnOverlayFailed(AndroidOverlay* overlay) {
  // We shouldn't get a failure for any overlay except the incoming one.
  DCHECK_EQ(overlay, overlay_.get());

  overlay_ = nullptr;
  most_recent_overlay_failure_ = tick_clock_->NowTicks();

  // If the client isn't already using a SurfaceTexture, then switch to it.
  // Note that this covers the case of kUnknown, when we might not have told
  // the client anything yet.  That's important for Initialize, so that a
  // failed overlay request still results in some callback to the client to
  // know what surface to start with.
  SwitchToSurfaceTexture();
}

void AndroidVideoSurfaceChooserImpl::OnOverlayDeleted(AndroidOverlay* overlay) {
  client_overlay_state_ = kUsingSurfaceTexture;
  // We don't call SwitchToSurfaceTexture since the client dropped the
  // overlay. It's already using SurfaceTexture.
}

}  // namespace media
