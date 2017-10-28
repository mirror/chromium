// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl3.h>
#include <stdint.h>

#include "gpu/command_buffer/tests/gl_manager.h"
#include "gpu/command_buffer/tests/gl_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gpu {

// A collection of tests that exercise the GL_EXT_srgb extension.
class GLEXTWindowRectanglesTest : public testing::Test {
 protected:
  void SetUp() override {
    GLManager::Options options;
    options.context_type = gles2::CONTEXT_TYPE_OPENGLES3;
    gl_.Initialize(options);
  }
  void TearDown() override { gl_.Destroy(); }
  bool IsApplicable() const {
    return GLTestHelper::HasExtension("GL_EXT_window_rectangles");
  }
  GLManager gl_;
};

TEST_F(GLEXTWindowRectanglesTest, Defaults) {
  GLint max = -1;
  {
    glGetIntegerv(GL_MAX_WINDOW_RECTANGLES_EXT, &max);
    EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());
    EXPECT_GE(max, 4);
  }

  {
    int mode = -1;
    glGetIntegerv(GL_WINDOW_RECTANGLE_MODE_EXT, &mode);
    EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());
    EXPECT_EQ(GL_EXCLUSIVE_EXT, mode);
  }
  {
    GLint num = -1;
    glGetIntegerv(GL_NUM_WINDOW_RECTANGLES_EXT, &num);
    EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());
    EXPECT_EQ(0, num);
  }

  for (int i = 0; i < max; ++i) {
    GLint rect[4] = {-1, -1, -1, -1};
    glGetIntegeri_v(GL_WINDOW_RECTANGLE_EXT, i, rect);
    EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());
    EXPECT_EQ(0, rect[0]);
    EXPECT_EQ(0, rect[1]);
    EXPECT_EQ(0, rect[2]);
    EXPECT_EQ(0, rect[3]);
  }
}

TEST_F(GLEXTWindowRectanglesTest, Defaults64) {
  int64_t max = -1;
  {
    glGetInteger64v(GL_MAX_WINDOW_RECTANGLES_EXT, &max);
    EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());
    EXPECT_GE(max, 4);
  }

  {
    int64_t mode = -1;
    glGetInteger64v(GL_WINDOW_RECTANGLE_MODE_EXT, &mode);
    EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());
    EXPECT_EQ(GL_EXCLUSIVE_EXT, mode);
  }
  {
    int64_t num = -1;
    glGetInteger64v(GL_NUM_WINDOW_RECTANGLES_EXT, &num);
    EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());
    EXPECT_EQ(0, num);
  }

  for (int i = 0; i < max; ++i) {
    int64_t rect[4] = {-1, -1, -1, -1};
    glGetInteger64i_v(GL_WINDOW_RECTANGLE_EXT, i, rect);
    EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());
    EXPECT_EQ(0, rect[0]);
    EXPECT_EQ(0, rect[1]);
    EXPECT_EQ(0, rect[2]);
    EXPECT_EQ(0, rect[3]);
  }
}

TEST_F(GLEXTWindowRectanglesTest, Basic) {
  GLint box_exp[12] = {};
  for (int i = 0; i < 12; ++i) {
    box_exp[i] = i;
  }
  glWindowRectanglesEXT(GL_INCLUSIVE_EXT, 3, box_exp);
  EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());

  {
    int mode = -1;
    glGetIntegerv(GL_WINDOW_RECTANGLE_MODE_EXT, &mode);
    EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());
    EXPECT_EQ(GL_INCLUSIVE_EXT, mode);
  }
  {
    GLint num = -1;
    glGetIntegerv(GL_NUM_WINDOW_RECTANGLES_EXT, &num);
    EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());
    EXPECT_EQ(3, num);
  }
  {
    GLint box[12] = {};
    for (int i = 0; i < 12; ++i) {
      box[i] = -1;
    }
    glGetIntegeri_v(GL_WINDOW_RECTANGLE_EXT, 0, &box[0]);
    glGetIntegeri_v(GL_WINDOW_RECTANGLE_EXT, 1, &box[4]);
    glGetIntegeri_v(GL_WINDOW_RECTANGLE_EXT, 2, &box[8]);
    EXPECT_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());
    for (int i = 0; i < 12; ++i) {
      EXPECT_EQ(box_exp[i], box[i]);
    }
  }
}

}  // namespace gpu
