// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/mojo/media_route_controller.h"

#include <utility>

#include "chrome/browser/media/router/media_router.h"

namespace media_router {

MediaRouteController::Observer::Observer(
    scoped_refptr<MediaRouteController> controller)
    : controller_(std::move(controller)) {
  controller_->AddObserver(this);
}

MediaRouteController::Observer::~Observer() {
  if (controller_)
    controller_->RemoveObserver(this);
}

void MediaRouteController::Observer::InvalidateController() {
  controller_ = nullptr;
  OnControllerInvalidated();
}

void MediaRouteController::Observer::OnControllerInvalidated() {}

MediaRouteController::MediaRouteController(const MediaRoute::Id& route_id,
                                           MediaRouter* media_router)
    : route_id_(route_id),
      mojo_media_controller_(),
      pending_controller_request_(mojo::MakeRequest(&mojo_media_controller_)),
      media_router_(media_router),
      binding_(this) {
  DCHECK(mojo_media_controller_.is_bound());
  DCHECK(media_router);
  mojo_media_controller_.set_connection_error_handler(base::BindOnce(
      &MediaRouteController::OnMojoConnectionError, base::Unretained(this)));
}

void MediaRouteController::InitAdditionalMojoConnnections() {}

RouteControllerType MediaRouteController::GetType() const {
  return RouteControllerType::GENERIC;
}

void MediaRouteController::Play() const {
  DCHECK(is_valid_);
  mojo_media_controller_->Play();
}

void MediaRouteController::Pause() const {
  DCHECK(is_valid_);
  mojo_media_controller_->Pause();
}

void MediaRouteController::Seek(base::TimeDelta time) const {
  DCHECK(is_valid_);
  mojo_media_controller_->Seek(time);
}

void MediaRouteController::SetMute(bool mute) const {
  DCHECK(is_valid_);
  mojo_media_controller_->SetMute(mute);
}

void MediaRouteController::SetVolume(float volume) const {
  DCHECK(is_valid_);
  mojo_media_controller_->SetVolume(volume);
}

void MediaRouteController::OnMediaStatusUpdated(const MediaStatus& status) {
  DCHECK(is_valid_);
  current_media_status_ = MediaStatus(status);
  for (Observer& observer : observers_)
    observer.OnMediaStatusUpdated(status);
}

void MediaRouteController::Invalidate() {
  is_valid_ = false;
  binding_.Close();
  mojo_media_controller_.reset();
  for (Observer& observer : observers_)
    observer.InvalidateController();
  // |this| is deleted here!
}

mojom::MediaControllerRequest MediaRouteController::PassControllerRequest() {
  return std::move(pending_controller_request_);
}

mojom::MediaStatusObserverPtr MediaRouteController::BindObserverPtr() {
  DCHECK(is_valid_);
  DCHECK(!binding_.is_bound());
  mojom::MediaStatusObserverPtr observer;
  binding_.Bind(mojo::MakeRequest(&observer));
  binding_.set_connection_error_handler(base::BindOnce(
      &MediaRouteController::OnMojoConnectionError, base::Unretained(this)));
  return observer;
}

MediaRouteController::~MediaRouteController() {
  if (is_valid_)
    media_router_->DetachRouteController(route_id_, this);
}

void MediaRouteController::AddObserver(Observer* observer) {
  DCHECK(is_valid_);
  observers_.AddObserver(observer);
}

void MediaRouteController::RemoveObserver(Observer* observer) {
  DCHECK(is_valid_);
  observers_.RemoveObserver(observer);
}

void MediaRouteController::OnMojoConnectionError() {
  media_router_->DetachRouteController(route_id_, this);
  Invalidate();
}

// static
const HangoutsMediaRouteController* HangoutsMediaRouteController::From(
    const MediaRouteController* controller) {
  if (!controller || controller->GetType() != RouteControllerType::HANGOUTS)
    return nullptr;

  return static_cast<const HangoutsMediaRouteController*>(controller);
}

// TODO(imcheng): Handle connection errors, invalidations etc.
HangoutsMediaRouteController::HangoutsMediaRouteController(
    const MediaRoute::Id& route_id,
    MediaRouter* media_router)
    : MediaRouteController(route_id, media_router),
      hangouts_controller_(),
      pending_hangouts_controller_request_(
          mojo::MakeRequest(&hangouts_controller_)) {
  hangouts_controller_.set_connection_error_handler(
      base::BindOnce(&HangoutsMediaRouteController::OnMojoConnectionError,
                     base::Unretained(this)));
}

HangoutsMediaRouteController::~HangoutsMediaRouteController() {}

void HangoutsMediaRouteController::InitAdditionalMojoConnnections() {
  MediaRouteController::InitAdditionalMojoConnnections();
  mojo_media_controller_->ConnectHangoutsMediaRouteController(
      std::move(pending_hangouts_controller_request_));
}

RouteControllerType HangoutsMediaRouteController::GetType() const {
  return RouteControllerType::HANGOUTS;
}

void HangoutsMediaRouteController::Invalidate() {
  hangouts_controller_.reset();
  MediaRouteController::Invalidate();
}

void HangoutsMediaRouteController::SetLocalPresent(bool local_present) const {
  hangouts_controller_->SetLocalPresent(local_present);
}

}  // namespace media_router
