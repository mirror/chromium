// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/media_controls/elements/MediaControlTimelineElement.h"

#include "core/events/PointerEvent.h"
#include "core/events/PointerEventInit.h"
#include "core/html/media/HTMLVideoElement.h"
#include "core/testing/PageTestBase.h"
#include "modules/media_controls/MediaControlsImpl.h"
#include "public/platform/WebPointerProperties.h"

namespace blink {

class MediaControlTimelineElementTest : public PageTestBase {
 public:
  static PointerEventInit GetValidPointerEventInit() {
    PointerEventInit init;
    init.setIsPrimary(true);
    init.setButton(static_cast<int>(WebPointerProperties::Button::kLeft));
    return init;
  }

  void SetUp() override {
    PageTestBase::SetUp();

    video_ = HTMLVideoElement::Create(GetDocument());
    timeline_ =
        new MediaControlTimelineElement(*(new MediaControlsImpl(*video_)));

    // Connects the timeline element. Ideally, we should be able to set the
    // NodeFlags::kConnectedFlag.
    GetDocument().body()->AppendChild(timeline_);

    // The video needs to be in the playing state in order to pretend
    Video()->Play();
    ASSERT_FALSE(Video()->paused());
  }

  HTMLVideoElement* Video() const { return video_; }

  MediaControlTimelineElement* Timeline() const { return timeline_; }

 private:
  Persistent<HTMLVideoElement> video_;
  Persistent<MediaControlTimelineElement> timeline_;
};

TEST_F(MediaControlTimelineElementTest, PointerDownPausesPlayback) {
  Timeline()->DispatchEvent(
      PointerEvent::Create("pointerdown", GetValidPointerEventInit()));
  EXPECT_TRUE(Video()->paused());
}

TEST_F(MediaControlTimelineElementTest, PointerDownRightClickNoOp) {
  PointerEventInit init = GetValidPointerEventInit();
  init.setButton(static_cast<int>(WebPointerProperties::Button::kRight));
  Timeline()->DispatchEvent(PointerEvent::Create("pointerdown", init));
  EXPECT_FALSE(Video()->paused());
}

TEST_F(MediaControlTimelineElementTest, PointerDownNotPrimaryNoOp) {
  PointerEventInit init = GetValidPointerEventInit();
  init.setIsPrimary(false);
  Timeline()->DispatchEvent(PointerEvent::Create("pointerdown", init));
  EXPECT_FALSE(Video()->paused());
}

TEST_F(MediaControlTimelineElementTest, PointerUpResumesPlayback) {
  Timeline()->DispatchEvent(
      PointerEvent::Create("pointerdown", GetValidPointerEventInit()));
  Timeline()->DispatchEvent(
      PointerEvent::Create("pointerup", GetValidPointerEventInit()));
  EXPECT_FALSE(Video()->paused());
}

TEST_F(MediaControlTimelineElementTest, PointerUpRightClickNoOp) {
  Timeline()->DispatchEvent(
      PointerEvent::Create("pointerdown", GetValidPointerEventInit()));

  PointerEventInit init = GetValidPointerEventInit();
  init.setButton(static_cast<int>(WebPointerProperties::Button::kRight));
  Timeline()->DispatchEvent(PointerEvent::Create("pointerup", init));
  EXPECT_TRUE(Video()->paused());
}

TEST_F(MediaControlTimelineElementTest, PointerUpNotPrimaryNoOp) {
  Timeline()->DispatchEvent(
      PointerEvent::Create("pointerdown", GetValidPointerEventInit()));

  PointerEventInit init = GetValidPointerEventInit();
  init.setIsPrimary(false);
  Timeline()->DispatchEvent(PointerEvent::Create("pointerup", init));
  EXPECT_TRUE(Video()->paused());
}

TEST_F(MediaControlTimelineElementTest, PointerOutDoesNotResume) {
  Timeline()->DispatchEvent(
      PointerEvent::Create("pointerdown", GetValidPointerEventInit()));
  Timeline()->DispatchEvent(
      PointerEvent::Create("pointerout", GetValidPointerEventInit()));
  EXPECT_TRUE(Video()->paused());
}

TEST_F(MediaControlTimelineElementTest, PointerMoveDoesNotResume) {
  Timeline()->DispatchEvent(
      PointerEvent::Create("pointerdown", GetValidPointerEventInit()));
  Timeline()->DispatchEvent(
      PointerEvent::Create("pointermove", GetValidPointerEventInit()));
  EXPECT_TRUE(Video()->paused());
}

TEST_F(MediaControlTimelineElementTest, PointerCancelResumesPlayback) {
  Timeline()->DispatchEvent(
      PointerEvent::Create("pointerdown", GetValidPointerEventInit()));
  Timeline()->DispatchEvent(
      PointerEvent::Create("pointercancel", GetValidPointerEventInit()));
  EXPECT_FALSE(Video()->paused());
}

}  // namespace blink
