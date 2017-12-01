// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/media_controls/elements/MediaControlPictureInPictureButtonElement.h"

#include "core/dom/events/Event.h"
#include "core/html/media/HTMLMediaElement.h"
#include "core/html/media/HTMLMediaSource.h"
#include "core/input_type_names.h"
#include "modules/media_controls/MediaControlsImpl.h"
#include "public/platform/Platform.h"
// #include "chrome/browser/picture_in_picture/picture_in_picture_window_controller.h"

namespace blink {

MediaControlPictureInPictureButtonElement::MediaControlPictureInPictureButtonElement(
    MediaControlsImpl& media_controls)
    : MediaControlInputElement(media_controls, kMediaPlayButton) {
  setType(InputTypeNames::button);
  SetShadowPseudoId(AtomicString("-webkit-media-controls-picture-in-picture-button"));
  LOG(ERROR) << "MediaControlPictureInPictureButtonElement";
}

bool MediaControlPictureInPictureButtonElement::WillRespondToMouseClickEvents() {
  return true;
}

void MediaControlPictureInPictureButtonElement::UpdateDisplayType() {
  SetDisplayType(kMediaMuteButton); // <--- this doesn't actually show up yet
  // SetClass("pause", MediaElement().paused());
  UpdateOverflowString();

  MediaControlInputElement::UpdateDisplayType();
}

WebLocalizedString::Name MediaControlPictureInPictureButtonElement::GetOverflowStringName()
    const {
  return WebLocalizedString::kOverflowMenuPictureInPicture;
}

bool MediaControlPictureInPictureButtonElement::HasOverflowButton() const {
  return true;
}

const char* MediaControlPictureInPictureButtonElement::GetNameForHistograms() const {
  // tools/metrics/histograms/histograms.xml @ PlayPauseOverflowButton
  // return IsOverflowElement() ? "PictureInPictureOverflowButton" : "PictureInPictureButton";
  return "";
}

void MediaControlPictureInPictureButtonElement::DefaultEventHandler(Event* event) {
  // LOG(ERROR) << "MediaControlPictureInPictureButtonElement::DefaultEventHandler";
  if (event->type() == EventTypeNames::click) {
    LOG(ERROR) << "MediaControlPictureInPictureButtonElement::DefaultEventHandler -> click";
    MediaElement().pictureInPicture();
  }

  // PictureInPictureWindowController* window_controller =
  //     // new PictureInPictureWindowController();
  //     PictureInPictureWindowController::Create();
  //     // PictureInPictureWindowController::GetOrCreateForWebContents(
  //         // embedder_web_contents_);
  // if (window_controller)
  //   LOG(ERROR) << "has window controller";
  // else
  //   LOG(ERROR) << "no window controller";
  // // window_controller->Show();

  MediaControlInputElement::DefaultEventHandler(event);
}

}  // namespace blink
