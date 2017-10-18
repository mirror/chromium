// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/textured_element.h"

#include "base/trace_event/trace_event.h"
#include "cc/paint/skia_paint_canvas.h"
#include "chrome/browser/vr/elements/ui_texture.h"
#include "chrome/browser/vr/ui_element_renderer.h"
#include "skia/ext/texture_handle.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "ui/gfx/geometry/rect_f.h"

namespace vr {

namespace {
static bool g_initialized_for_testing_ = false;
static constexpr bool kUseGanesh = true;
}

TexturedElement::TexturedElement(int maximum_width)
    : texture_handle_(unsigned(-1)), maximum_width_(maximum_width) {}

TexturedElement::~TexturedElement() = default;

void TexturedElement::Initialize(VrSurfaceProvider* provider) {
  TRACE_EVENT0("gpu", "TexturedElement::Initialize");
  texture_size_ = GetTexture()->GetPreferredTextureSize(maximum_width_);
  if (kUseGanesh) {
    DCHECK_NE(nullptr, provider);
    provider_ = provider;
  } else {
    glGenTextures(1, &texture_handle_);
    DCHECK(GetTexture() != nullptr);
  }
  GetTexture()->OnInitialized();
  initialized_ = true;
  UpdateTexture();
}

// static
void TexturedElement::SetInitializedForTesting() {
  g_initialized_for_testing_ = true;
}

void TexturedElement::UpdateTexture() {
  if (!initialized_ || !GetTexture()->dirty() || !IsVisible())
    return;

  if (kUseGanesh) {
    surface_ =
        provider_->MakeSurface(texture_size_.width(), texture_size_.height());
    DCHECK_NE(nullptr, surface_.get());
  } else {
    surface_ = SkSurface::MakeRasterN32Premul(texture_size_.width(),
                                              texture_size_.height());
  }

  GetTexture()->DrawAndLayout(surface_->getCanvas(), texture_size_);
  Flush(surface_.get());
  // Update the element size's aspect ratio to match the texture.
  UpdateElementSize();
}

void TexturedElement::UpdateElementSize() {
  // Updating the height according to width is a hack.  This may be overridden.
  gfx::SizeF drawn_size = GetTexture()->GetDrawnSize();
  float height =
      drawn_size.height() / drawn_size.width() * stale_size().width();
  SetSize(stale_size().width(), height);
}

void TexturedElement::Render(
    UiElementRenderer* renderer,
    const gfx::Transform& model_view_proj_matrix) const {
  if (!g_initialized_for_testing_) {
    if (!initialized_)
      return;
    DCHECK(!GetTexture()->dirty());
  }
  gfx::SizeF drawn_size = GetTexture()->GetDrawnSize();
  gfx::RectF copy_rect(0, 0, drawn_size.width() / texture_size_.width(),
                       drawn_size.height() / texture_size_.height());
  renderer->DrawTexturedQuad(texture_handle_,
                             UiElementRenderer::kTextureLocationLocal,
                             model_view_proj_matrix, copy_rect,
                             computed_opacity(), size(), corner_radius());
}

void TexturedElement::Flush(SkSurface* surface) {
  if (kUseGanesh) {
    surface->getCanvas()->flush();
    GrBackendObject backend_object =
        surface->getTextureHandle(SkSurface::kFlushRead_BackendHandleAccess);
    DCHECK_NE(backend_object, 0);
    texture_handle_ =
        skia::GrBackendObjectToGrGLTextureInfo(backend_object)->fID;
    DCHECK_NE(texture_handle_, 0u);
    LOG(INFO) << "TEXTURE 2 " << texture_handle_;
  } else {
    cc::SkiaPaintCanvas paint_canvas(surface->getCanvas());
    paint_canvas.flush();
    SkPixmap pixmap;
    CHECK(surface->peekPixels(&pixmap));

    SkColorType type = pixmap.colorType();
    DCHECK(type == kRGBA_8888_SkColorType || type == kBGRA_8888_SkColorType);
    GLint format = (type == kRGBA_8888_SkColorType ? GL_RGBA : GL_BGRA);

    glBindTexture(GL_TEXTURE_2D, texture_handle_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, format, pixmap.width(), pixmap.height(), 0,
                 format, GL_UNSIGNED_BYTE, pixmap.addr());
  }
}

void TexturedElement::OnSetMode() {
  GetTexture()->SetMode(mode());
  UpdateTexture();
}

void TexturedElement::PrepareToDraw() {
  UpdateTexture();
}

}  // namespace vr
