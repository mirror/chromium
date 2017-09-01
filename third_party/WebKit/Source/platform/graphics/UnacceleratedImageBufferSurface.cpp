/*
 * Copyright (c) 2013, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "platform/graphics/UnacceleratedImageBufferSurface.h"

#include "platform/graphics/StaticBitmapImage.h"
#include "platform/graphics/skia/SkiaUtils.h"
#include "platform/wtf/PassRefPtr.h"
#include "third_party/skia/include/core/SkSurface.h"

namespace blink {
static float Float16ToFloat(const uint16_t& f16) {
  union FloatUIntUnion {
    uint32_t fUInt;
    float fFloat;
  };
  FloatUIntUnion magic = {126 << 23};
  FloatUIntUnion o;
  if (((f16 >> 10) & 0x001f) == 0) {
    o.fUInt = magic.fUInt + (f16 & 0x03ff);
    o.fFloat -= magic.fFloat;
  } else {
    o.fUInt = (f16 & 0x03ff) << 13;
    if (((f16 >> 10) & 0x001f) == 0x1f)
      o.fUInt |= (255 << 23);
    else
      o.fUInt |= ((127 - 15 + ((f16 >> 10) & 0x001f)) << 23);
  }
  o.fUInt |= ((f16 >> 15) << 31);
  return o.fFloat;
}

static void PrintSkImage(sk_sp<SkImage> input, int errorId = 0) {
  // find out the color space
  SkColorSpace* color_space = input->colorSpace();
  std::stringstream cstr;
  if (!color_space)
    cstr << "Legacy Color Space";
  else if (SkColorSpace::Equals(color_space, SkColorSpace::MakeSRGB().get()))
    cstr << "SRGB Color Space";
  else if (SkColorSpace::Equals(color_space,
                                SkColorSpace::MakeSRGBLinear().get()))
    cstr << "Linear SRGB Color Space";
  else if (SkColorSpace::Equals(
               color_space,
               SkColorSpace::MakeRGB(SkColorSpace::kLinear_RenderTargetGamma,
                                     SkColorSpace::kDCIP3_D65_Gamut)
                   .get()))
    cstr << "P3 Color Space";
  else if (SkColorSpace::Equals(
               color_space,
               SkColorSpace::MakeRGB(SkColorSpace::kLinear_RenderTargetGamma,
                                     SkColorSpace::kRec2020_Gamut)
                   .get()))
    cstr << "Rec2020 Color Space";
  cstr << ", ";
  if (input->alphaType() == kPremul_SkAlphaType)
    cstr << "Premul";
  else
    cstr << "Unpremul";

  LOG(ERROR) << "Printing SkImage @" << errorId << " :" << input->width() << ","
             << input->height() << " @ " << cstr.str();
  SkColorType color_type = kN32_SkColorType;
  if (input->colorSpace() && input->refColorSpace()->gammaIsLinear())
    color_type = kRGBA_F16_SkColorType;
  SkImageInfo info =
      SkImageInfo::Make(input->width(), input->height(), color_type,
                        input->alphaType(), input->refColorSpace());

  std::stringstream str;
  if (info.bytesPerPixel() == 4) {
    std::unique_ptr<uint8_t[]> read_pixels(
        new uint8_t[input->width() * input->height() * info.bytesPerPixel()]());
    input->readPixels(info, read_pixels.get(),
                      input->width() * info.bytesPerPixel(), 0, 0);

    for (int i = 0; i < input->width() * input->height(); i++) {
      if (i > 0 && i % input->width() == 0)
        str << "\n";
      str << "[";
      for (int j = 0; j < info.bytesPerPixel(); j++)
        str << (int)(read_pixels[i * info.bytesPerPixel() + j]) << ", ";
      str << " ]";
    }
  } else {
    std::unique_ptr<uint16_t[]> read_pixels(
        new uint16_t[input->width() * input->height() * 4]());
    input->readPixels(info, read_pixels.get(),
                      input->width() * info.bytesPerPixel(), 0, 0);

    for (int i = 0; i < input->width() * input->height(); i++) {
      if (i > 0 && i % input->width() == 0)
        str << "\n";
      str << "[";
      for (int j = 0; j < 4; j++)
        str << Float16ToFloat(read_pixels[i * 4 + j]) << ", ";
      str << " ]";
    }
  }
  LOG(ERROR) << str.str();
}

UnacceleratedImageBufferSurface::UnacceleratedImageBufferSurface(
    const IntSize& size,
    OpacityMode opacity_mode,
    ImageInitializationMode initialization_mode,
    const CanvasColorParams& color_params)
    : ImageBufferSurface(size, opacity_mode, color_params) {
  SkAlphaType alpha_type =
      (kOpaque == opacity_mode) ? kOpaque_SkAlphaType : kPremul_SkAlphaType;
  SkImageInfo info = SkImageInfo::Make(
      size.Width(), size.Height(), color_params.GetSkColorType(), alpha_type);
  // In legacy mode the backing SkSurface should not have any color space.
  // If color correct rendering is enabled only for SRGB, still the backing
  // surface should not have any color space and the treatment of legacy data
  // as SRGB will be managed by wrapping the internal SkCanvas inside a
  // SkColorSpaceXformCanvas. If color correct rendering is enbaled for other
  // color spaces, we set the color space properly.
  if (CanvasColorParams::ColorCorrectRenderingInAnyColorSpace())
    info = info.makeColorSpace(color_params.GetSkColorSpaceForSkSurfaces());

  SkSurfaceProps disable_lcd_props(0, kUnknown_SkPixelGeometry);
  surface_ = SkSurface::MakeRaster(
      info, kOpaque == opacity_mode ? 0 : &disable_lcd_props);
  if (!surface_)
    return;

  sk_sp<SkImage> image = surface_->makeImageSnapshot();
  PrintSkImage(image, 170);
  SkPaint paint;
  paint.setColor(SkPackARGB32(0xFF, 0xFF, 0x80, 0x1B));
  surface_->getCanvas()->drawRect(SkRect::MakeWH(2,2), paint);
  image = surface_->makeImageSnapshot();
  PrintSkImage(image, 170);

  sk_sp<SkColorSpace> xform_canvas_color_space = nullptr;
  if (color_params.ColorCorrectNoColorSpaceToSRGB())
    xform_canvas_color_space = color_params.GetSkColorSpace();
  canvas_ = WTF::WrapUnique(
      new SkiaPaintCanvas(surface_->getCanvas(), xform_canvas_color_space));

  // Always save an initial frame, to support resetting the top level matrix
  // and clip.
  canvas_->save();

  if (initialization_mode == kInitializeImagePixels)
    Clear();
}

UnacceleratedImageBufferSurface::~UnacceleratedImageBufferSurface() {}

PaintCanvas* UnacceleratedImageBufferSurface::Canvas() {
  return canvas_.get();
}

bool UnacceleratedImageBufferSurface::IsValid() const {
  return surface_;
}

bool UnacceleratedImageBufferSurface::WritePixels(const SkImageInfo& orig_info,
                                                  const void* pixels,
                                                  size_t row_bytes,
                                                  int x,
                                                  int y) {
  return surface_->getCanvas()->writePixels(orig_info, pixels, row_bytes, x, y);
}

RefPtr<StaticBitmapImage> UnacceleratedImageBufferSurface::NewImageSnapshot(
    AccelerationHint,
    SnapshotReason) {
  return StaticBitmapImage::Create(surface_->makeImageSnapshot());
}

}  // namespace blink
