// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/testing/PageTestBase.h"
#include "modules/picture_in_picture/PictureInPictureController.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

class PictureInPictureTest : public PageTestBase {
 protected:
  void SetUp() final {
    // Create page with video to run tests on.
    PageTestBase::SetUp();
    SetHtmlInnerHTML("<body><video></video></body>");
  }

  // HTMLVideoElement* VideoElement() {
  //   return ToHTMLVideoElement(GetDocument().QuerySelector("video"));
  // }
};

// TEST_F(PictureInPictureTest,
// RequestPictureInPictureRejectsWhenPictureInPictureEnabledIsFalse) {
//   PictureInPictureController* controller =
//   PictureInPictureController::Ensure(GetDocument());
//   controller.SetPictureInPictureEnabledForTesting(false);

//   ScriptState* script_state =
//   ToScriptStateForMainWorld(GetDocument().GetFrame()); ScriptPromise promise
//   = HTMLVideoElementPictureInPicture.requestPictureInPicture(script_state,
//   VideoElement())

// }

TEST_F(PictureInPictureTest,
       PictureInPictureEnabledReturnsFalseWhenPictureInPictureEnabledIsFalse) {
  Persistent<PictureInPictureController> controller =
      PictureInPictureController::Ensure(GetDocument());
  controller->SetPictureInPictureEnabledForTesting(false);
  EXPECT_FALSE(controller->PictureInPictureEnabled());
}

}  // namespace blink
