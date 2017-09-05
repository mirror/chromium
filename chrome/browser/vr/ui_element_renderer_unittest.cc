// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/ui_element_renderer.h"

#include "base/macros.h"

#include "base/memory/ptr_util.h"
#include "chrome/browser/vr/elements/content_element.h"
#include "chrome/browser/vr/elements/draw_phase.h"
#include "chrome/browser/vr/elements/grid.h"
#include "chrome/browser/vr/elements/rect.h"
#include "chrome/browser/vr/elements/textured_element.h"
#include "chrome/browser/vr/elements/ui_texture.h"
#include "chrome/browser/vr/test/animation_utils.h"
#include "chrome/browser/vr/test/constants.h"
#include "chrome/browser/vr/test/fake_ui_element_renderer.h"
#include "chrome/browser/vr/test/mock_content_input_delegate.h"
#include "chrome/browser/vr/ui_scene.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/size_f.h"

namespace vr {

class FakeUiTexture : public UiTexture {
 public:
  FakeUiTexture() : UiTexture() { UiTexture::dirty_ = false; }

  ~FakeUiTexture() override {}

  gfx::Size GetPreferredTextureSize(int maximum_width) const override {
    return gfx::Size();
  }

  gfx::SizeF GetDrawnSize() const override { return gfx::SizeF(); }

 protected:
  void Draw(SkCanvas* canvas, const gfx::Size& texture_size) override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(FakeUiTexture);
};

class FakeTexturedElement : public TexturedElement {
 public:
  FakeTexturedElement()
      : TexturedElement(512), texture_(base::MakeUnique<FakeUiTexture>()) {
    TexturedElement::initialized_ = true;
  }

  ~FakeTexturedElement() override {}

 private:
  UiTexture* GetTexture() const override { return texture_.get(); }

  std::unique_ptr<FakeUiTexture> texture_;

  DISALLOW_COPY_AND_ASSIGN(FakeTexturedElement);
};

TEST(UiElementRendererTest, ElementsApplyParentOpacity) {
  UiScene scene;
  FakeUiElementRenderer renderer;
  MockContentInputDelegate delegate;

  auto parent_element = base::MakeUnique<UiElement>();
  UiElement* parent = parent_element.get();
  parent_element->set_draw_phase(kPhaseForeground);
  scene.AddUiElement(kRoot, std::move(parent_element));

  auto content_element = base::MakeUnique<ContentElement>(&delegate);
  ContentElement* content = content_element.get();
  content_element->set_draw_phase(kPhaseForeground);
  content_element->set_texture_id(1);
  parent->AddChild(std::move(content_element));

  auto grid_element = base::MakeUnique<Grid>();
  Grid* grid = grid_element.get();
  grid_element->set_draw_phase(kPhaseForeground);
  parent->AddChild(std::move(grid_element));

  auto rect_element = base::MakeUnique<Rect>();
  rect_element->set_draw_phase(kPhaseForeground);
  Rect* rect = rect_element.get();
  parent->AddChild(std::move(rect_element));

  auto textured_element = base::MakeUnique<FakeTexturedElement>();
  textured_element->set_draw_phase(kPhaseForeground);
  TexturedElement* texture = textured_element.get();
  parent->AddChild(std::move(textured_element));

  content->Render(&renderer, kProjMatrix);
  EXPECT_FLOAT_EQ(renderer.texture_opacity(), content->computed_opacity());
  texture->Render(&renderer, kProjMatrix);
  EXPECT_FLOAT_EQ(renderer.texture_opacity(), texture->computed_opacity());
  grid->Render(&renderer, kProjMatrix);
  EXPECT_FLOAT_EQ(renderer.grid_opacity(), grid->computed_opacity());
  rect->Render(&renderer, kProjMatrix);
  EXPECT_FLOAT_EQ(renderer.grid_opacity(), rect->computed_opacity());

  // Change parent opacity.
  parent->SetOpacity(0.2);
  gfx::Vector3dF look_at{0.f, 0.f, -1.f};
  scene.OnBeginFrame(MicrosecondsToTicks(0), look_at);
  content->Render(&renderer, kProjMatrix);
  EXPECT_FLOAT_EQ(renderer.texture_opacity(), content->computed_opacity());
  texture->Render(&renderer, kProjMatrix);
  EXPECT_FLOAT_EQ(renderer.texture_opacity(), texture->computed_opacity());
  grid->Render(&renderer, kProjMatrix);
  EXPECT_FLOAT_EQ(renderer.grid_opacity(), grid->computed_opacity());
  rect->Render(&renderer, kProjMatrix);
  EXPECT_FLOAT_EQ(renderer.grid_opacity(), rect->computed_opacity());
}

}  // namespace vr
