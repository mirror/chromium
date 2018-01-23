// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/render_text_test_api.h"

#include "ui/gfx/render_text_harfbuzz.h"

namespace gfx {
namespace test {

RenderTextHarfBuzz* RenderTextTestApi::harf_buzz() const {
  return static_cast<RenderTextHarfBuzz*>(render_text_);
}

const internal::TextRunList* RenderTextTestApi::GetHarfBuzzRunList() const {
  return harf_buzz()->GetRunList();
}

void RenderTextTestApi::SetGlyphWidth(float width) {
  harf_buzz()->set_glyph_width_for_test(width);
}

}  // namespace test
}  // namespace gfx
