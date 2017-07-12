// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/imagebitmap/ImageBitmap.h"

#include <memory>
#include "core/html/HTMLCanvasElement.h"
#include "core/html/HTMLVideoElement.h"
#include "core/html/ImageData.h"
#include "core/offscreencanvas/OffscreenCanvas.h"
#include "platform/graphics/CanvasColorParams.h"
#include "platform/graphics/skia/SkiaUtils.h"
#include "platform/image-decoders/ImageDecoder.h"
#include "platform/wtf/CheckedNumeric.h"
#include "platform/wtf/PtrUtil.h"
#include "platform/wtf/RefPtr.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkColorSpaceXformCanvas.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "third_party/skia/include/core/SkSwizzle.h"

namespace blink {

constexpr const char* kImageOrientationFlipY = "flipY";
constexpr const char* kImageBitmapOptionNone = "none";
constexpr const char* kImageBitmapOptionDefault = "default";
constexpr const char* kImageBitmapOptionPremultiply = "premultiply";
constexpr const char* kImageBitmapOptionResizeQualityHigh = "high";
constexpr const char* kImageBitmapOptionResizeQualityMedium = "medium";
constexpr const char* kImageBitmapOptionResizeQualityPixelated = "pixelated";
constexpr const char* kSRGBImageBitmapColorSpaceConversion = "srgb";
constexpr const char* kLinearRGBImageBitmapColorSpaceConversion = "linear-rgb";
constexpr const char* kP3ImageBitmapColorSpaceConversion = "p3";
constexpr const char* kRec2020ImageBitmapColorSpaceConversion = "rec2020";

namespace {

static bool Logging() {
  return false;
}

static void PrintSkImage(sk_sp<SkImage> input, int errorId = 0) {
  if (!Logging())
    return;
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
  std::unique_ptr<uint8_t[]> read_pixels(
      new uint8_t[input->width() * input->height() * info.bytesPerPixel()]());
  input->readPixels(info, read_pixels.get(),
                    input->width() * info.bytesPerPixel(), 0, 0);
  for (int i = 0; i < input->width() * input->height(); i++) {
    if (i > 0 && i % input->width() == 0)
      str << "\n";
    str << "[";
    for (int j = 0; j < info.bytesPerPixel(); j++)
      str << (int)(read_pixels[i * info.bytesPerPixel() + j]) << ",";
    str << "] ";
  }
  LOG(ERROR) << str.str();
}

static void PrintImageData(ImageData* input, int errorId = 0) {
  if (!Logging())
    return;

  // find out the color space
  SkColorSpace* color_space =
      input->GetCanvasColorParams().GetSkColorSpaceForSkSurfaces().get();
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

  LOG(ERROR) << "Printing ImageData @" << errorId << " :" << input->width()
             << "," << input->height() << " @ " << cstr.str();
  unsigned char* data =
      static_cast<unsigned char*>(input->BufferBase()->Data());
  std::stringstream str;
  for (int i = 0; i < input->width() * input->height(); i++) {
    if (i > 0 && i % input->width() == 0)
      str << "\n";
    str << "[";
    for (int j = 0; j < 4; j++)
      str << (int)(data[i * 4 + j]) << ",";
    str << "] ";
  }
  LOG(ERROR) << str.str();
}

static void PrintPixels(const void* input,
                        int width,
                        int height,
                        int pixel_size = 4,
                        int errorId = 0) {
  if (!Logging())
    return;
  LOG(ERROR) << "Printing Pixels @" << errorId << " :" << width << ","
             << height;
  const unsigned char* data = static_cast<const unsigned char*>(input);
  std::stringstream str;
  for (int i = 0; i < width * height; i++) {
    if (i % width == 0)
      str << "\n";
    str << "[";
    for (int j = 0; j < pixel_size; j++)
      str << (int)(data[i * pixel_size + j]) << ",";
    str << "] ";
  }
  LOG(ERROR) << str.str();
}

struct ParsedOptions {
  bool flip_y = false;
  bool premultiply_alpha = true;
  bool should_scale_input = false;
  unsigned resize_width = 0;
  unsigned resize_height = 0;
  IntRect crop_rect;
  SkFilterQuality resize_quality = kLow_SkFilterQuality;
  CanvasColorParams color_params;
  bool color_canvas_extensions_enabled = false;
};

// The following two functions are helpers used in cropImage
static inline IntRect NormalizeRect(const IntRect& rect) {
  return IntRect(std::min(rect.X(), rect.MaxX()),
                 std::min(rect.Y(), rect.MaxY()),
                 std::max(rect.Width(), -rect.Width()),
                 std::max(rect.Height(), -rect.Height()));
}

ParsedOptions ParseOptions(const ImageBitmapOptions& options,
                           Optional<IntRect> crop_rect,
                           IntSize source_size) {
  ParsedOptions parsed_options;
  if (options.imageOrientation() == kImageOrientationFlipY) {
    parsed_options.flip_y = true;
  } else {
    parsed_options.flip_y = false;
    DCHECK(options.imageOrientation() == kImageBitmapOptionNone);
  }
  if (options.premultiplyAlpha() == kImageBitmapOptionNone) {
    parsed_options.premultiply_alpha = false;
  } else {
    parsed_options.premultiply_alpha = true;
    DCHECK(options.premultiplyAlpha() == kImageBitmapOptionDefault ||
           options.premultiplyAlpha() == kImageBitmapOptionPremultiply);
  }

  if (options.colorSpaceConversion() != kImageBitmapOptionNone) {
    parsed_options.color_canvas_extensions_enabled =
        RuntimeEnabledFeatures::ColorCanvasExtensionsEnabled();
    if (!parsed_options.color_canvas_extensions_enabled) {
      DCHECK_EQ(options.colorSpaceConversion(), kImageBitmapOptionDefault);
      if (RuntimeEnabledFeatures::ColorCorrectRenderingEnabled()) {
        parsed_options.color_params.SetCanvasColorSpace(kSRGBCanvasColorSpace);
        parsed_options.color_canvas_extensions_enabled = true;
      } else {
        parsed_options.color_params.SetCanvasColorSpace(
            kLegacyCanvasColorSpace);
      }
    } else {
      if (options.colorSpaceConversion() == kImageBitmapOptionDefault ||
          options.colorSpaceConversion() ==
              kSRGBImageBitmapColorSpaceConversion) {
        parsed_options.color_params.SetCanvasColorSpace(kSRGBCanvasColorSpace);
      } else if (options.colorSpaceConversion() ==
                 kLinearRGBImageBitmapColorSpaceConversion) {
        parsed_options.color_params.SetCanvasColorSpace(kSRGBCanvasColorSpace);
        parsed_options.color_params.SetCanvasPixelFormat(kF16CanvasPixelFormat);
      } else if (options.colorSpaceConversion() ==
                 kP3ImageBitmapColorSpaceConversion) {
        parsed_options.color_params.SetCanvasColorSpace(kP3CanvasColorSpace);
        parsed_options.color_params.SetCanvasPixelFormat(kF16CanvasPixelFormat);
      } else if (options.colorSpaceConversion() ==
                 kRec2020ImageBitmapColorSpaceConversion) {
        parsed_options.color_params.SetCanvasColorSpace(
            kRec2020CanvasColorSpace);
        parsed_options.color_params.SetCanvasPixelFormat(kF16CanvasPixelFormat);
      } else {
        NOTREACHED()
            << "Invalid ImageBitmap creation attribute colorSpaceConversion: "
            << options.colorSpaceConversion();
      }
    }
  }

  int source_width = source_size.Width();
  int source_height = source_size.Height();
  if (!crop_rect) {
    parsed_options.crop_rect = IntRect(0, 0, source_width, source_height);
  } else {
    parsed_options.crop_rect = NormalizeRect(*crop_rect);
  }
  if (!options.hasResizeWidth() && !options.hasResizeHeight()) {
    parsed_options.resize_width = parsed_options.crop_rect.Width();
    parsed_options.resize_height = parsed_options.crop_rect.Height();
  } else if (options.hasResizeWidth() && options.hasResizeHeight()) {
    parsed_options.resize_width = options.resizeWidth();
    parsed_options.resize_height = options.resizeHeight();
  } else if (options.hasResizeWidth() && !options.hasResizeHeight()) {
    parsed_options.resize_width = options.resizeWidth();
    parsed_options.resize_height = ceil(
        static_cast<float>(options.resizeWidth()) /
        parsed_options.crop_rect.Width() * parsed_options.crop_rect.Height());
  } else {
    parsed_options.resize_height = options.resizeHeight();
    parsed_options.resize_width = ceil(
        static_cast<float>(options.resizeHeight()) /
        parsed_options.crop_rect.Height() * parsed_options.crop_rect.Width());
  }
  if (static_cast<int>(parsed_options.resize_width) ==
          parsed_options.crop_rect.Width() &&
      static_cast<int>(parsed_options.resize_height) ==
          parsed_options.crop_rect.Height()) {
    parsed_options.should_scale_input = false;
    return parsed_options;
  }
  parsed_options.should_scale_input = true;

  if (options.resizeQuality() == kImageBitmapOptionResizeQualityHigh)
    parsed_options.resize_quality = kHigh_SkFilterQuality;
  else if (options.resizeQuality() == kImageBitmapOptionResizeQualityMedium)
    parsed_options.resize_quality = kMedium_SkFilterQuality;
  else if (options.resizeQuality() == kImageBitmapOptionResizeQualityPixelated)
    parsed_options.resize_quality = kNone_SkFilterQuality;
  else
    parsed_options.resize_quality = kLow_SkFilterQuality;
  return parsed_options;
}

// The function dstBufferSizeHasOverflow() is being called at the beginning of
// each ImageBitmap() constructor, which makes sure that doing
// width * height * bytesPerPixel will never overflow unsigned.
bool DstBufferSizeHasOverflow(const ParsedOptions& options) {
  CheckedNumeric<unsigned> total_bytes = options.crop_rect.Width();
  total_bytes *= options.crop_rect.Height();
  total_bytes *=
      SkColorTypeBytesPerPixel(options.color_params.GetSkColorType());
  if (!total_bytes.IsValid())
    return true;

  if (!options.should_scale_input)
    return false;
  total_bytes = options.resize_width;
  total_bytes *= options.resize_height;
  total_bytes *=
      SkColorTypeBytesPerPixel(options.color_params.GetSkColorType());
  if (!total_bytes.IsValid())
    return true;

  return false;
}

}  // namespace

static SkColorType GetSkImageSkColorType(sk_sp<SkImage> image) {
  SkColorType color_type = kN32_SkColorType;
  if (image->colorSpace() && image->refColorSpace()->gammaIsLinear())
    color_type = kRGBA_F16_SkColorType;
  return color_type;
}

static SkImageInfo GetSkImageSkImageInfo(sk_sp<SkImage> image) {
  return SkImageInfo::Make(image->width(), image->height(),
                           GetSkImageSkColorType(image), image->alphaType(),
                           image->refColorSpace());
}

static PassRefPtr<Uint8Array> CopySkImageData(sk_sp<SkImage> input,
                                              SkImageInfo info) {
  unsigned width = static_cast<unsigned>(input->width());
  RefPtr<ArrayBuffer> dst_buffer =
      ArrayBuffer::CreateOrNull(width * input->height(), info.bytesPerPixel());
  if (!dst_buffer)
    return nullptr;
  unsigned byte_length = dst_buffer->ByteLength();
  RefPtr<Uint8Array> dst_pixels =
      Uint8Array::Create(std::move(dst_buffer), 0, byte_length);
  input->readPixels(info, dst_pixels->Data(), width * info.bytesPerPixel(), 0,
                    0);
  return dst_pixels;
}

static PassRefPtr<Uint8Array> CopySkImageData(sk_sp<SkImage> input) {
  SkImageInfo info = SkImageInfo::Make(
      input->width(), input->height(), GetSkImageSkColorType(input),
      input->alphaType(), input->refColorSpace());
  return CopySkImageData(input, info);
}

static sk_sp<SkImage> NewSkImageFromRaster(
    const SkImageInfo& info,
    PassRefPtr<Uint8Array> image_pixels) {
  unsigned image_row_bytes = info.width() * info.bytesPerPixel();
  SkPixmap pixmap(info, image_pixels->Data(), image_row_bytes);
  auto rasterReleaseProc = [](const void*, void* pixels) {
    static_cast<Uint8Array*>(pixels)->Deref();
  };
  return SkImage::MakeFromRaster(pixmap, rasterReleaseProc,
                                 image_pixels.LeakRef());
}

static sk_sp<SkImage> FlipSkImageVertically(sk_sp<SkImage> input) {
  RefPtr<Uint8Array> image_pixels = CopySkImageData(input);
  if (!image_pixels)
    return nullptr;
  SkImageInfo info = GetSkImageSkImageInfo(input);
  unsigned image_row_bytes = info.width() * info.bytesPerPixel();
  for (int i = 0; i < info.height() / 2; i++) {
    unsigned top_first_element = i * image_row_bytes;
    unsigned top_last_element = (i + 1) * image_row_bytes;
    unsigned bottom_first_element = (info.height() - 1 - i) * image_row_bytes;
    std::swap_ranges(image_pixels->Data() + top_first_element,
                     image_pixels->Data() + top_last_element,
                     image_pixels->Data() + bottom_first_element);
  }
  return NewSkImageFromRaster(info, std::move(image_pixels));
}

static sk_sp<SkImage> GetSkImageWithAlphaDisposition(
    sk_sp<SkImage> image,
    AlphaDisposition alpha_disposition) {
  SkAlphaType alpha_type = kPremul_SkAlphaType;
  if (alpha_disposition == kDontPremultiplyAlpha)
    alpha_type = kUnpremul_SkAlphaType;
  if (image->alphaType() == alpha_type)
    return image;

  SkImageInfo info = GetSkImageSkImageInfo(image);
  PrintPixels(CopySkImageData(image, info)->Data(), image->width(),
              image->height(), 4, 350);
  info = info.makeAlphaType(alpha_type);
  RefPtr<Uint8Array> dst_pixels = CopySkImageData(image, info);
  PrintPixels(dst_pixels->Data(), image->width(), image->height(), 4, 350);
  if (!dst_pixels)
    return nullptr;
  return NewSkImageFromRaster(info, std::move(dst_pixels));
}

static sk_sp<SkImage> ConvertFromNoColorSpaceToSRGB(sk_sp<SkImage> image) {
  DCHECK(!image->colorSpace());
  PrintSkImage(image, 365);
  RefPtr<Uint8Array> dst_pixels = CopySkImageData(image);
  PrintPixels(dst_pixels->Data(), image->width(), image->height(), 4, 370);
  if (!dst_pixels)
    return nullptr;
  SkImageInfo info = SkImageInfo::Make(
      image->width(), image->height(), GetSkImageSkColorType(image),
      image->alphaType(), SkColorSpace::MakeSRGB());
  return NewSkImageFromRaster(info, std::move(dst_pixels));
}

static sk_sp<SkImage> ScaleSkImage(sk_sp<SkImage> image,
                                   const unsigned& resize_width,
                                   const unsigned& resize_height,
                                   const SkFilterQuality& resize_quality) {
  bool image_tagged_premul = false;
  SkImageInfo info = GetSkImageSkImageInfo(image);
  // if image is unpremul, just tag it to premul for now.
  if (image->alphaType() == kUnpremul_SkAlphaType) {
    info = info.makeAlphaType(kPremul_SkAlphaType);
    RefPtr<Uint8Array> pixels = CopySkImageData(image);
    image = NewSkImageFromRaster(info, std::move(pixels));
    image_tagged_premul = true;
    PrintSkImage(image, 390);
  }

  RefPtr<ArrayBuffer> dst_buffer = ArrayBuffer::CreateOrNull(
      resize_width * resize_height, info.bytesPerPixel());
  if (!dst_buffer)
    return nullptr;

  RefPtr<Uint8Array> resized_pixels =
      Uint8Array::Create(dst_buffer, 0, dst_buffer->ByteLength());
  SkPixmap pixmap(info.makeWH(resize_width, resize_height),
                  resized_pixels->Data(), resize_width * info.bytesPerPixel());
  image->scalePixels(pixmap, resize_quality);
  auto rasterReleaseProc = [](const void*, void* pixels) {
    static_cast<Uint8Array*>(pixels)->Deref();
  };
  sk_sp<SkImage> scaled_image = SkImage::MakeFromRaster(
      pixmap, rasterReleaseProc, resized_pixels.LeakRef());
  PrintSkImage(scaled_image, 410);
  // if the image was tagged premul, re-tag scaled_image to unpremul.
  if (image_tagged_premul) {
    info = GetSkImageSkImageInfo(scaled_image)
               .makeAlphaType(kUnpremul_SkAlphaType);
    RefPtr<Uint8Array> pixels = CopySkImageData(scaled_image);
    scaled_image = NewSkImageFromRaster(info, std::move(pixels));
  }
  return scaled_image;
}

static void ApplyColorSpaceConversion(sk_sp<SkImage>& image,
                                      ParsedOptions& options) {
  if (options.color_params.UsesOutputSpaceBlending() &&
      RuntimeEnabledFeatures::ColorCorrectRenderingEnabled()) {
    image = image->makeColorSpace(options.color_params.GetSkColorSpace(),
                                  SkTransferFunctionBehavior::kIgnore);
  }
  if (!options.color_canvas_extensions_enabled)
    return;

  // if there is no color space information, treat pixels as they are in SRGB.
  if (!image->colorSpace())
    image = ConvertFromNoColorSpaceToSRGB(image);

  sk_sp<SkColorSpace> dst_color_space =
      options.color_params.GetSkColorSpaceForSkSurfaces();
  DCHECK(dst_color_space.get());
  if (SkColorSpace::Equals(image->colorSpace(), dst_color_space.get()))
    return;

  SkImageInfo dst_info = SkImageInfo::Make(
      image->width(), image->height(), options.color_params.GetSkColorType(),
      image->alphaType(), dst_color_space);
  size_t size = image->width() * image->height() * dst_info.bytesPerPixel();
  sk_sp<SkData> dst_data = SkData::MakeUninitialized(size);
  if (dst_data->size() != size)
    return;
  if (image->readPixels(dst_info, dst_data->writable_data(),
                        image->width() * dst_info.bytesPerPixel(), 0, 0)) {
    image = SkImage::MakeRasterData(dst_info, dst_data,
                                    image->width() * dst_info.bytesPerPixel());
    return;
  }
  NOTREACHED();
  return;
}

sk_sp<SkImage> ImageBitmap::GetSkImageFromDecoder(
    std::unique_ptr<ImageDecoder> decoder,
    SkColorType* decoded_color_type,
    sk_sp<SkColorSpace>* decoded_color_space) {
  if (!decoder->FrameCount())
    return nullptr;
  ImageFrame* frame = decoder->FrameBufferAtIndex(0);
  if (!frame || frame->GetStatus() != ImageFrame::kFrameComplete)
    return nullptr;
  DCHECK(!frame->Bitmap().isNull() && !frame->Bitmap().empty());
  sk_sp<SkImage> image = frame->FinalizePixelsAndGetImage();
  if (decoded_color_type)
    *decoded_color_type = frame->Bitmap().colorType();
  if (decoded_color_space)
    *decoded_color_space = sk_sp<SkColorSpace>(frame->Bitmap().colorSpace());
  return image;
}

bool ImageBitmap::IsResizeOptionValid(const ImageBitmapOptions& options,
                                      ExceptionState& exception_state) {
  if ((options.hasResizeWidth() && options.resizeWidth() == 0) ||
      (options.hasResizeHeight() && options.resizeHeight() == 0)) {
    exception_state.ThrowDOMException(
        kInvalidStateError,
        "The resizeWidth or/and resizeHeight is equal to 0.");
    return false;
  }
  return true;
}

bool ImageBitmap::IsSourceSizeValid(int source_width,
                                    int source_height,
                                    ExceptionState& exception_state) {
  if (!source_width || !source_height) {
    exception_state.ThrowDOMException(
        kIndexSizeError, String::Format("The source %s provided is 0.",
                                        source_width ? "height" : "width"));
    return false;
  }
  return true;
}

static PassRefPtr<StaticBitmapImage> CreateTransparentBlackImage(
    const ParsedOptions& parsed_options) {
  SkImageInfo info = SkImageInfo::Make(
      parsed_options.crop_rect.Width(), parsed_options.crop_rect.Height(),
      parsed_options.color_params.GetSkColorType(), kPremul_SkAlphaType,
      parsed_options.color_params.GetSkColorSpaceForSkSurfaces());
  if (parsed_options.should_scale_input) {
    info =
        info.makeWH(parsed_options.resize_width, parsed_options.resize_height);
  }
  sk_sp<SkSurface> surface = SkSurface::MakeRaster(info);
  if (!surface)
    return nullptr;
  return StaticBitmapImage::Create(surface->makeImageSnapshot());
}

static PassRefPtr<StaticBitmapImage> CropImageAndApplyColorSpaceConversion(
    RefPtr<Image> image,
    ParsedOptions& parsed_options,
    ColorBehavior color_behavior = ColorBehavior::TransformToGlobalTarget()) {
  DCHECK(image);
  IntRect img_rect(IntPoint(), IntSize(image->width(), image->height()));
  const IntRect src_rect = Intersection(img_rect, parsed_options.crop_rect);

  // If cropRect doesn't intersect the source image, return a transparent black
  // image.
  if (src_rect.IsEmpty())
    return CreateTransparentBlackImage(parsed_options);

  sk_sp<SkImage> skia_image = image->ImageForCurrentFrame();
  PrintSkImage(skia_image, 525);
  // Attempt to get raw unpremultiplied image data, executed only when
  // skia_image is premultiplied.
  if ((((!image->IsSVGImage() && !skia_image->isOpaque()) || !skia_image) &&
       image->Data() && skia_image->alphaType() == kPremul_SkAlphaType) ||
      color_behavior.IsIgnore()) {
    std::unique_ptr<ImageDecoder> decoder(ImageDecoder::Create(
        image->Data(), true,
        parsed_options.premultiply_alpha ? ImageDecoder::kAlphaPremultiplied
                                         : ImageDecoder::kAlphaNotPremultiplied,
        color_behavior));
    if (!decoder)
      return nullptr;
    SkColorType color_type = parsed_options.color_params.GetSkColorType();
    sk_sp<SkColorSpace> color_space =
        parsed_options.color_params.GetSkColorSpace();
    skia_image = ImageBitmap::GetSkImageFromDecoder(std::move(decoder),
                                                    &color_type, &color_space);
    if (!skia_image)
      return nullptr;
  }

  if (src_rect != img_rect)
    skia_image = skia_image->makeSubset(src_rect);

  // flip if needed
  if (parsed_options.flip_y)
    skia_image = FlipSkImageVertically(skia_image);
  PrintSkImage(skia_image, 550);

  // premultiply / unpremultiply if needed
  skia_image = GetSkImageWithAlphaDisposition(
      skia_image, parsed_options.premultiply_alpha ? kPremultiplyAlpha
                                                   : kDontPremultiplyAlpha);
  PrintSkImage(skia_image, 555);

  // resize if needed
  if (parsed_options.should_scale_input) {
    skia_image = ScaleSkImage(skia_image, parsed_options.resize_width,
                              parsed_options.resize_height,
                              parsed_options.resize_quality);
  }
  PrintSkImage(skia_image, 560);

  // color correct the image
  ApplyColorSpaceConversion(skia_image, parsed_options);
  PrintSkImage(skia_image, 570);

  return StaticBitmapImage::Create(std::move(skia_image));
}

ImageBitmap::ImageBitmap(ImageElementBase* image,
                         Optional<IntRect> crop_rect,
                         Document* document,
                         const ImageBitmapOptions& options) {
  RefPtr<Image> input = image->CachedImage()->GetImage();
  ParsedOptions parsed_options =
      ParseOptions(options, crop_rect, image->BitmapSourceSize());
  if (DstBufferSizeHasOverflow(parsed_options))
    return;

  image_ = CropImageAndApplyColorSpaceConversion(
      input, parsed_options,
      options.colorSpaceConversion() == kImageBitmapOptionNone
          ? ColorBehavior::Ignore()
          : ColorBehavior::TransformToGlobalTarget());
  if (!image_)
    return;

  // In the case where the source image is lazy-decoded, m_image may not be in
  // a decoded state, we trigger it here.
  sk_sp<SkImage> sk_image = image_->ImageForCurrentFrame();
  SkPixmap pixmap;
  if (!sk_image->isTextureBacked() && !sk_image->peekPixels(&pixmap)) {
    sk_sp<SkColorSpace> dst_color_space = nullptr;
    SkColorType dst_color_type = kN32_SkColorType;
    if (parsed_options.color_canvas_extensions_enabled ||
        (parsed_options.color_params.UsesOutputSpaceBlending() &&
         RuntimeEnabledFeatures::ColorCorrectRenderingEnabled())) {
      dst_color_space = parsed_options.color_params.GetSkColorSpace();
      dst_color_type = parsed_options.color_params.GetSkColorType();
    }
    SkImageInfo image_info =
        SkImageInfo::Make(sk_image->width(), sk_image->height(), dst_color_type,
                          kPremul_SkAlphaType, dst_color_space);
    sk_sp<SkSurface> surface = SkSurface::MakeRaster(image_info);
    surface->getCanvas()->drawImage(sk_image, 0, 0);
    sk_sp<SkImage> skia_image = surface->makeImageSnapshot();
    ApplyColorSpaceConversion(skia_image, parsed_options);
    image_ = StaticBitmapImage::Create(std::move(skia_image));
  }
  if (!image_)
    return;

  image_->SetOriginClean(
      !image->WouldTaintOrigin(document->GetSecurityOrigin()));
  image_->SetPremultiplied(parsed_options.premultiply_alpha);
}

ImageBitmap::ImageBitmap(HTMLVideoElement* video,
                         Optional<IntRect> crop_rect,
                         Document* document,
                         const ImageBitmapOptions& options) {
  ParsedOptions parsed_options =
      ParseOptions(options, crop_rect, video->BitmapSourceSize());
  if (DstBufferSizeHasOverflow(parsed_options))
    return;

  std::unique_ptr<ImageBuffer> buffer =
      ImageBuffer::Create(IntSize(video->videoWidth(), video->videoHeight()),
                          kNonOpaque, kDoNotInitializeImagePixels);
  if (!buffer)
    return;

  video->PaintCurrentFrame(
      buffer->Canvas(),
      IntRect(IntPoint(), IntSize(video->videoWidth(), video->videoHeight())),
      nullptr);
  RefPtr<Image> input = buffer->NewImageSnapshot();
  image_ = CropImageAndApplyColorSpaceConversion(input, parsed_options);
  if (!image_)
    return;

  image_->SetOriginClean(
      !video->WouldTaintOrigin(document->GetSecurityOrigin()));
  image_->SetPremultiplied(parsed_options.premultiply_alpha);
}

ImageBitmap::ImageBitmap(HTMLCanvasElement* canvas,
                         Optional<IntRect> crop_rect,
                         const ImageBitmapOptions& options) {
  DCHECK(canvas->IsPaintable());
  RefPtr<Image> input;
  if (canvas->PlaceholderFrame()) {
    input = canvas->PlaceholderFrame();
  } else {
    input = canvas->CopiedImage(kBackBuffer, kPreferAcceleration,
                                kSnapshotReasonCreateImageBitmap);
  }
  PrintSkImage(input->ImageForCurrentFrame(), 655);

  ParsedOptions parsed_options = ParseOptions(
      options, crop_rect, IntSize(input->width(), input->height()));
  if (DstBufferSizeHasOverflow(parsed_options))
    return;

  image_ = CropImageAndApplyColorSpaceConversion(input, parsed_options);
  if (!image_)
    return;

  image_->SetOriginClean(canvas->OriginClean());
  image_->SetPremultiplied(parsed_options.premultiply_alpha);
}

ImageBitmap::ImageBitmap(OffscreenCanvas* offscreen_canvas,
                         Optional<IntRect> crop_rect,
                         const ImageBitmapOptions& options) {
  SourceImageStatus status;
  RefPtr<Image> input = offscreen_canvas->GetSourceImageForCanvas(
      &status, kPreferNoAcceleration, kSnapshotReasonCreateImageBitmap,
      FloatSize(offscreen_canvas->Size()));
  if (status != kNormalSourceImageStatus)
    return;

  ParsedOptions parsed_options = ParseOptions(
      options, crop_rect, IntSize(input->width(), input->height()));
  if (DstBufferSizeHasOverflow(parsed_options))
    return;

  image_ = CropImageAndApplyColorSpaceConversion(input, parsed_options);
  if (!image_)
    return;
  image_->SetOriginClean(offscreen_canvas->OriginClean());
  image_->SetPremultiplied(parsed_options.premultiply_alpha);
}

ImageBitmap::ImageBitmap(const void* pixel_data,
                         uint32_t width,
                         uint32_t height,
                         bool is_image_bitmap_premultiplied,
                         bool is_image_bitmap_origin_clean) {
  SkImageInfo info = SkImageInfo::MakeN32(width, height,
                                          is_image_bitmap_premultiplied
                                              ? kPremul_SkAlphaType
                                              : kUnpremul_SkAlphaType);
  SkPixmap pixmap(info, pixel_data, info.bytesPerPixel() * width);
  image_ = StaticBitmapImage::Create(SkImage::MakeRasterCopy(pixmap));
  if (!image_)
    return;
  image_->SetPremultiplied(is_image_bitmap_premultiplied);
  image_->SetOriginClean(is_image_bitmap_origin_clean);
}

static void SwizzleImageData(ImageData* data) {
  if (!data || (kN32_SkColorType != kBGRA_8888_SkColorType) ||
      (data->GetCanvasColorParams().GetSkColorSpaceForSkSurfaces() &&
       data->GetCanvasColorParams()
           .GetSkColorSpaceForSkSurfaces()
           ->gammaIsLinear()))
    return;
  SkSwapRB(reinterpret_cast<uint32_t*>(data->data()->Data()),
           reinterpret_cast<uint32_t*>(data->data()->Data()),
           data->Size().Height() * data->Size().Width());
}

ImageBitmap::ImageBitmap(ImageData* data,
                         Optional<IntRect> crop_rect,
                         const ImageBitmapOptions& options) {
  ParsedOptions parsed_options =
      ParseOptions(options, crop_rect, data->BitmapSourceSize());
  if (DstBufferSizeHasOverflow(parsed_options))
    return;

  PrintImageData(data, 720);
  IntRect data_src_rect = IntRect(IntPoint(), data->Size());
  IntRect src_rect = crop_rect
                         ? Intersection(parsed_options.crop_rect, data_src_rect)
                         : data_src_rect;

  // If cropRect doesn't intersect the source image, return a transparent black
  // image.
  if (src_rect.IsEmpty()) {
    image_ = CreateTransparentBlackImage(parsed_options);
    return;
  }

  PrintImageData(data, 735);
  // crop/flip the input if needed
  bool crop_or_flip = (src_rect != data_src_rect) || parsed_options.flip_y;
  ImageData* cropped_data = data;
  if (crop_or_flip)
    cropped_data = data->CropRect(src_rect, parsed_options.flip_y);
  PrintImageData(data, 740);

  // swizzle if needed
  SwizzleImageData(cropped_data);
  PrintImageData(data, 745);

  // create a SkImage fom ImageData
  int byte_length = cropped_data->BufferBase()->ByteLength();
  RefPtr<ArrayBuffer> array_buffer = ArrayBuffer::CreateOrNull(byte_length, 1);
  if (!array_buffer)
    return;
  RefPtr<Uint8Array> image_pixels =
      Uint8Array::Create(std::move(array_buffer), 0, byte_length);
  memcpy(image_pixels->Data(), cropped_data->BufferBase()->Data(), byte_length);

  SkImageInfo info = SkImageInfo::Make(
      cropped_data->width(), cropped_data->height(),
      cropped_data->GetCanvasColorParams().GetSkColorType(),
      kUnpremul_SkAlphaType,
      cropped_data->GetCanvasColorParams().GetSkColorSpaceForSkSurfaces());

  sk_sp<SkImage> skia_image = NewSkImageFromRaster(info, image_pixels);
  PrintSkImage(skia_image, 760);

  // swizzle back
  SwizzleImageData(cropped_data);
  PrintSkImage(skia_image, 765);

  // premultiply if needed
  if (parsed_options.premultiply_alpha)
    skia_image = GetSkImageWithAlphaDisposition(skia_image, kPremultiplyAlpha);
  PrintSkImage(skia_image, 770);

  // color correct if needed
  ApplyColorSpaceConversion(skia_image, parsed_options);
  PrintSkImage(skia_image, 775);

  // resize if needed
  if (parsed_options.should_scale_input) {
    skia_image = ScaleSkImage(skia_image, parsed_options.resize_width,
                              parsed_options.resize_height,
                              parsed_options.resize_quality);
  }
  PrintSkImage(skia_image, 780);

  image_ = StaticBitmapImage::Create(std::move(skia_image));
  PrintSkImage(image_->ImageForCurrentFrame(), 785);
  image_->SetPremultiplied(parsed_options.premultiply_alpha);
}

ImageBitmap::ImageBitmap(ImageBitmap* bitmap,
                         Optional<IntRect> crop_rect,
                         const ImageBitmapOptions& options) {
  RefPtr<Image> input = bitmap->BitmapImage();
  if (!input)
    return;
  ParsedOptions parsed_options =
      ParseOptions(options, crop_rect, input->Size());
  if (DstBufferSizeHasOverflow(parsed_options))
    return;

  image_ = CropImageAndApplyColorSpaceConversion(input, parsed_options);
  if (!image_)
    return;
  image_->SetOriginClean(bitmap->OriginClean());
  image_->SetPremultiplied(parsed_options.premultiply_alpha);
}

ImageBitmap::ImageBitmap(RefPtr<StaticBitmapImage> image,
                         Optional<IntRect> crop_rect,
                         const ImageBitmapOptions& options) {
  bool origin_clean = image->OriginClean();
  ParsedOptions parsed_options =
      ParseOptions(options, crop_rect, image->Size());
  if (DstBufferSizeHasOverflow(parsed_options))
    return;

  image_ = CropImageAndApplyColorSpaceConversion(image, parsed_options);
  if (!image_)
    return;

  image_->SetOriginClean(origin_clean);
  image_->SetPremultiplied(parsed_options.premultiply_alpha);
}

ImageBitmap::ImageBitmap(PassRefPtr<StaticBitmapImage> image) {
  image_ = std::move(image);
}

PassRefPtr<StaticBitmapImage> ImageBitmap::Transfer() {
  DCHECK(!IsNeutered());
  is_neutered_ = true;
  image_->Transfer();
  return std::move(image_);
}

ImageBitmap::~ImageBitmap() {}

ImageBitmap* ImageBitmap::Create(ImageElementBase* image,
                                 Optional<IntRect> crop_rect,
                                 Document* document,
                                 const ImageBitmapOptions& options) {
  return new ImageBitmap(image, crop_rect, document, options);
}

ImageBitmap* ImageBitmap::Create(HTMLVideoElement* video,
                                 Optional<IntRect> crop_rect,
                                 Document* document,
                                 const ImageBitmapOptions& options) {
  return new ImageBitmap(video, crop_rect, document, options);
}

ImageBitmap* ImageBitmap::Create(HTMLCanvasElement* canvas,
                                 Optional<IntRect> crop_rect,
                                 const ImageBitmapOptions& options) {
  return new ImageBitmap(canvas, crop_rect, options);
}

ImageBitmap* ImageBitmap::Create(OffscreenCanvas* offscreen_canvas,
                                 Optional<IntRect> crop_rect,
                                 const ImageBitmapOptions& options) {
  return new ImageBitmap(offscreen_canvas, crop_rect, options);
}

ImageBitmap* ImageBitmap::Create(ImageData* data,
                                 Optional<IntRect> crop_rect,
                                 const ImageBitmapOptions& options) {
  return new ImageBitmap(data, crop_rect, options);
}

ImageBitmap* ImageBitmap::Create(ImageBitmap* bitmap,
                                 Optional<IntRect> crop_rect,
                                 const ImageBitmapOptions& options) {
  return new ImageBitmap(bitmap, crop_rect, options);
}

ImageBitmap* ImageBitmap::Create(PassRefPtr<StaticBitmapImage> image,
                                 Optional<IntRect> crop_rect,
                                 const ImageBitmapOptions& options) {
  return new ImageBitmap(std::move(image), crop_rect, options);
}

ImageBitmap* ImageBitmap::Create(PassRefPtr<StaticBitmapImage> image) {
  return new ImageBitmap(std::move(image));
}

ImageBitmap* ImageBitmap::Create(const void* pixel_data,
                                 uint32_t width,
                                 uint32_t height,
                                 bool is_image_bitmap_premultiplied,
                                 bool is_image_bitmap_origin_clean) {
  return new ImageBitmap(pixel_data, width, height,
                         is_image_bitmap_premultiplied,
                         is_image_bitmap_origin_clean);
}

void ImageBitmap::close() {
  if (!image_ || is_neutered_)
    return;
  image_.Clear();
  is_neutered_ = true;
}

// static
ImageBitmap* ImageBitmap::Take(ScriptPromiseResolver*, sk_sp<SkImage> image) {
  return ImageBitmap::Create(StaticBitmapImage::Create(std::move(image)));
}

PassRefPtr<Uint8Array> ImageBitmap::CopyBitmapData(AlphaDisposition alpha_op,
                                                   DataColorFormat format) {
  SkImageInfo info = SkImageInfo::Make(
      width(), height(),
      (format == kRGBAColorType) ? kRGBA_8888_SkColorType : kN32_SkColorType,
      (alpha_op == kPremultiplyAlpha) ? kPremul_SkAlphaType
                                      : kUnpremul_SkAlphaType);
  RefPtr<Uint8Array> dst_pixels =
      CopySkImageData(image_->ImageForCurrentFrame(), info);
  return dst_pixels;
}

unsigned long ImageBitmap::width() const {
  if (!image_)
    return 0;
  DCHECK_GT(image_->width(), 0);
  return image_->width();
}

unsigned long ImageBitmap::height() const {
  if (!image_)
    return 0;
  DCHECK_GT(image_->height(), 0);
  return image_->height();
}

bool ImageBitmap::IsAccelerated() const {
  return image_ && (image_->IsTextureBacked() || image_->HasMailbox());
}

IntSize ImageBitmap::Size() const {
  if (!image_)
    return IntSize();
  DCHECK_GT(image_->width(), 0);
  DCHECK_GT(image_->height(), 0);
  return IntSize(image_->width(), image_->height());
}

ScriptPromise ImageBitmap::CreateImageBitmap(ScriptState* script_state,
                                             EventTarget& event_target,
                                             Optional<IntRect> crop_rect,
                                             const ImageBitmapOptions& options,
                                             ExceptionState& exception_state) {
  if ((crop_rect && !IsSourceSizeValid(crop_rect->Width(), crop_rect->Height(),
                                       exception_state)) ||
      !IsSourceSizeValid(width(), height(), exception_state))
    return ScriptPromise();
  if (!IsResizeOptionValid(options, exception_state))
    return ScriptPromise();
  return ImageBitmapSource::FulfillImageBitmap(
      script_state, Create(this, crop_rect, options));
}

PassRefPtr<Image> ImageBitmap::GetSourceImageForCanvas(
    SourceImageStatus* status,
    AccelerationHint,
    SnapshotReason,
    const FloatSize&) {
  *status = kNormalSourceImageStatus;
  if (!image_)
    return nullptr;
  if (image_->IsPremultiplied())
    return image_;
  // Skia does not support drawing unpremul SkImage on SkCanvas.
  // Premultiply and return.
  PrintSkImage(image_->ImageForCurrentFrame(), 1015);
  sk_sp<SkImage> premul_sk_image = GetSkImageWithAlphaDisposition(
      image_->ImageForCurrentFrame(), kPremultiplyAlpha);
  PrintSkImage(premul_sk_image, 1020);
  return StaticBitmapImage::Create(premul_sk_image);
}

void ImageBitmap::AdjustDrawRects(FloatRect* src_rect,
                                  FloatRect* dst_rect) const {}

FloatSize ImageBitmap::ElementSize(const FloatSize&) const {
  return FloatSize(width(), height());
}

DEFINE_TRACE(ImageBitmap) {}

}  // namespace blink
