// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/html/media/PictureInPictureInterstitial.h"

#include "core/dom/TaskRunnerHelper.h"
#include "core/html/HTMLImageElement.h"
#include "core/html/media/HTMLVideoElement.h"
#include "core/html/media/PictureInPictureElements.h"
#include "platform/text/PlatformLocale.h"
#include "public/platform/WebLocalizedString.h"

namespace {

// Used with |interstitial_timer_| to allow interstitial to appear or disappear.
constexpr double kInterval = 0.0;

}  // namespace

namespace blink {

PictureInPictureInterstitial::PictureInPictureInterstitial(
    HTMLVideoElement& videoElement)
    : HTMLDivElement(videoElement.GetDocument()),
      insterstitial_timer_(
          TaskRunnerHelper::Get(TaskType::kUnthrottled,
                                &videoElement.GetDocument()),
          this,
          &PictureInPictureInterstitial::ToggleInterstitialTimerFired),
      video_element_(&videoElement) {
  SetShadowPseudoId(AtomicString("-internal-picture-in-picture-interstitial"));
  background_image_ = HTMLImageElement::Create(videoElement.GetDocument());
  background_image_->SetShadowPseudoId(
      AtomicString("-internal-picture-in-picture-background-image"));
  background_image_->SetSrc(videoElement.getAttribute(HTMLNames::posterAttr));
  AppendChild(background_image_);

  message_element_ = new PictureInPictureMessageElement(*this);
  message_element_->setInnerText(
      GetVideoElement().GetLocale().QueryString(
          WebLocalizedString::kMediaRemotingCastToUnknownDeviceText),
      ASSERT_NO_EXCEPTION);
  AppendChild(message_element_);
}

void PictureInPictureInterstitial::Show() {
  if (should_be_visible_)
    return;

  if (insterstitial_timer_.IsActive())
    insterstitial_timer_.Stop();

  should_be_visible_ = true;
  RemoveInlineStyleProperty(CSSPropertyDisplay);
  insterstitial_timer_.StartOneShot(kInterval, BLINK_FROM_HERE);
}

void PictureInPictureInterstitial::Hide() {
  if (!should_be_visible_)
    return;

  if (insterstitial_timer_.IsActive())
    insterstitial_timer_.Stop();

  should_be_visible_ = false;
  SetInlineStyleProperty(CSSPropertyOpacity, 0,
                         CSSPrimitiveValue::UnitType::kNumber);
  insterstitial_timer_.StartOneShot(kInterval, BLINK_FROM_HERE);
}

void PictureInPictureInterstitial::ToggleInterstitialTimerFired(TimerBase*) {
  insterstitial_timer_.Stop();
  if (should_be_visible_) {
    SetInlineStyleProperty(CSSPropertyOpacity, 1,
                           CSSPrimitiveValue::UnitType::kNumber);
  } else {
    SetInlineStyleProperty(CSSPropertyDisplay, CSSValueNone);
  }
}

void PictureInPictureInterstitial::Trace(blink::Visitor* visitor) {
  visitor->Trace(video_element_);
  visitor->Trace(background_image_);
  visitor->Trace(message_element_);
  HTMLDivElement::Trace(visitor);
}

}  // namespace blink
