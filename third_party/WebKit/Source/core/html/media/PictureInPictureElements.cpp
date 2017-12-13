// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/html/media/PictureInPictureElements.h"

#include "core/dom/ShadowRoot.h"

namespace blink {

PictureInPictureMessageElement::PictureInPictureMessageElement(
    PictureInPictureInterstitial& interstitial)
    : HTMLDivElement(interstitial.GetDocument()) {
  SetShadowPseudoId(AtomicString("-internal-picture-in-picture-message"));
}

}  // namespace blink