// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "cc/base/lap_timer.h"
#include "cc/paint/skia_paint_canvas.h"
#include "chrome/browser/vr/elements/text_texture.h"
#include "chrome/browser/vr/test/constants.h"
#include "chrome/browser/vr/test/gl_test_environment.h"
#include "chrome/browser/vr/test/ui_pixel_test.h"
#include "chrome/browser/vr/toolbar_state.h"
#include "chrome/grit/generated_resources.h"
#include "components/toolbar/vector_icons.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/perf/perf_test.h"
#include "third_party/skia/include/core/SkSurface.h"

namespace vr {

TEST(TextTexturePerfTest, Test) {
  gfx::Size frame_buffer_size(960, 1080);
  auto gl_test_environment =
      base::MakeUnique<GlTestEnvironment>(frame_buffer_size);

  GLuint texture_handle;
  glGenTextures(1, &texture_handle);

  std::unique_ptr<UiTexture> texture = base::MakeUnique<TextTexture>(
      base::UTF8ToUTF16(kLoremIpsum700Chars), 0.05f, 1.0f);
  gfx::Size texture_size = texture->GetPreferredTextureSize(512);
  sk_sp<SkSurface> surface = SkSurface::MakeRasterN32Premul(
      texture_size.width(), texture_size.height());
  cc::SkiaPaintCanvas paint_canvas(surface->getCanvas());
  SkPixmap pixmap;

  cc::LapTimer timer;
  timer.Start();
  for (size_t i = 0; i < 35; i++) {
    texture->DrawAndLayout(surface->getCanvas(), texture_size);
    paint_canvas.flush();
    CHECK(surface->peekPixels(&pixmap));

    SkColorType type = pixmap.colorType();
    DCHECK(type == kRGBA_8888_SkColorType || type == kBGRA_8888_SkColorType);
    GLint format = (type == kRGBA_8888_SkColorType ? GL_RGBA : GL_BGRA);

    glBindTexture(GL_TEXTURE_2D, texture_handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, format, pixmap.width(), pixmap.height(), 0,
                 format, GL_UNSIGNED_BYTE, pixmap.addr());

    glFinish();
    timer.NextLap();
  }
  perf_test::PrintResult("render_lorem_ipsum_700_chars", "", "render_time_avg",
                         timer.MsPerLap(), "ms", true);
  perf_test::PrintResult("render_lorem_ipsum_700_chars", "", "number_of_runs",
                         static_cast<size_t>(timer.NumLaps()), "runs", true);
}

}  // namespace vr
