// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <stdint.h>

#include "gpu/command_buffer/tests/gl_manager.h"
#include "gpu/command_buffer/tests/gl_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gpu {

namespace {

class GLOOBAttribTest : public testing::Test {
 protected:
  void SetUp() override { gl_.Initialize(GLManager::Options()); }
  void TearDown() override { gl_.Destroy(); }
  GLManager gl_;
};

TEST_F(GLOOBAttribTest, DrawUsingOOBDisabledArrays) {
  static const char* v_shader_str =
      "attribute mat3 att3;\n"
      "attribute mat4 att4;\n"
      "varying vec4 color;\n"
      "void main ()\n"
      "{\n"
      "        color = vec4( 1.0, \n"
      "                      att3[0][0] + att3[0][1] + att3[0][2] + att3[1][0] + att3[1][1] + att3[1][2] + att3[2][0] + att3[2][1] + att3[2][2],  \n"
      "                     att4[0][0] + att4[0][1] + att4[0][2] + att4[0][3] + att4[1][0] + att4[1][1] + att4[1][2] + att4[1][3] + att4[2][0] + att4[2][1] + att4[2][2] + att4[2][3] + att4[3][0] + att4[3][1] + att4[3][2] + att4[3][3], 10500000000 );\n"
      "}\n";
      static const char* f_shader_str =
      "precision mediump float;\n"
      "void main()\n"
      "{\n"
      "}\n";

  GLuint program = GLTestHelper::LoadProgram(v_shader_str, f_shader_str);
  DCHECK(program);
  glUseProgram(program);
  GLuint vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, false, 0, nullptr);
  glEnableVertexAttribArray(1);
  glDrawArrays(GL_TRIANGLES, 0, 450000000);
  GLenum expected = GL_INVALID_OPERATION;
  EXPECT_EQ(expected, glGetError());
}

}  // anonymous namespace

}  // namespace gpu

