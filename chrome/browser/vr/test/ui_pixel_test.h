// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_TEST_UI_PIXEL_TEST_H_
#define CHROME_BROWSER_VR_TEST_UI_PIXEL_TEST_H_

#include "base/files/file_path.h"
#include "chrome/browser/vr/model/toolbar_state.h"
#include "chrome/browser/vr/test/gl_test_environment.h"
#include "chrome/browser/vr/test/mock_content_input_delegate.h"
#include "chrome/browser/vr/test/mock_ui_browser_interface.h"
#include "chrome/browser/vr/ui.h"
#include "chrome/browser/vr/ui_input_manager.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/transform.h"

namespace vr {

// Base class for GTest-based VR pixel tests. Tests inheriting from this class
// are meant to be used alongside the Skia Gold image diff service and the
// Python wrapper at //testing/scripts/run_gold_test.py. See that file and
// https://skia.org/dev/testing/skiagold for more specifics on Gold and its
// integration with these tests.

// The general test flow is to render the VR browser and save the produced
// frame buffer. The wrapper script will then compare the image to baselines and
// fail if a difference is detected.
class UiPixelTest : public testing::Test {
 public:
  UiPixelTest();
  ~UiPixelTest() override;
  void SetUp() override;
  void TearDown() override;

 protected:
  // Generates a blank content quad texture filled with the given color
  GLuint MakeSolidContentTexture(SkColor color);
  // Generates VR browser UI state using the given states, but does not draw
  void MakeUi(const UiInitialState& ui_initial_state,
              const ToolbarState& toolbar_state);
  // Draws the UI into the frame buffer using the current UI state and provided
  // laser/controller/view state. Only renders the left eye.
  void DrawUi(const gfx::Vector3dF& laser_direction,
              const gfx::Point3F& laser_origin,
              UiInputManager::ButtonState button_state,
              float controller_opacity,
              const gfx::Transform& controller_transform,
              const gfx::Transform& view_matrix,
              const gfx::Transform& proj_matrix);
  // Saves whatever is currently in the frame buffer to a new SkBitmap
  std::unique_ptr<SkBitmap> SaveCurrentFrameBufferToSkBitmap();
  // Captures the current frame buffer and saves it as a PNG to the Gold output
  // directory provided to the test.
  void CaptureAndSaveFrameBufferToGoldDir();
  // Saves the given SkBitmap to a PNG file at the specified path.
  bool SaveSkBitmapToPng(const SkBitmap& bitmap, const std::string& filename);

 private:
  base::FilePath image_output_directory_;
  std::unique_ptr<GlTestEnvironment> gl_test_environment_;
  std::unique_ptr<MockUiBrowserInterface> browser_;
  std::unique_ptr<MockContentInputDelegate> content_input_delegate_;
  GLuint content_texture_ = 0;
  gfx::Size frame_buffer_size_;
  std::unique_ptr<Ui> ui_;
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_TEST_UI_PIXEL_TEST_H_
