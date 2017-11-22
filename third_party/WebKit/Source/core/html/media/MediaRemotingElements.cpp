// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/html/media/MediaRemotingElements.h"

#include "core/dom/ShadowRoot.h"
#include "public/platform/WebLocalizedString.h"

namespace blink {

// ----------------------------

MediaRemotingCastMessageElement::MediaRemotingCastMessageElement(
    MediaRemotingInterstitial& interstitial)
    : HTMLDivElement(interstitial.GetDocument()) {
  SetShadowPseudoId(AtomicString("-internal-media-remoting-cast-text-message"));
}

// ----------------------------

MediaRemotingCastIconElement::MediaRemotingCastIconElement(
    MediaRemotingInterstitial& interstitial)
    : HTMLDivElement(interstitial.GetDocument()) {
  SetShadowPseudoId(AtomicString("-internal-media-remoting-cast-icon"));
}

}  // namespace blink
