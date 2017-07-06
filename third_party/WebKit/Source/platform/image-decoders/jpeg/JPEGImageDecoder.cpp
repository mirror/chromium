/*
 * Copyright (C) 2006 Apple Computer, Inc.
 *
 * Portions are Copyright (C) 2001-6 mozilla.org
 *
 * Other contributors:
 *   Stuart Parmenter <stuart@mozilla.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * Alternatively, the contents of this file may be used under the terms
 * of either the Mozilla Public License Version 1.1, found at
 * http://www.mozilla.org/MPL/ (the "MPL") or the GNU General Public
 * License Version 2.0, found at http://www.fsf.org/copyleft/gpl.html
 * (the "GPL"), in which case the provisions of the MPL or the GPL are
 * applicable instead of those above.  If you wish to allow use of your
 * version of this file only under the terms of one of those two
 * licenses (the MPL or the GPL) and not to allow others to use your
 * version of this file under the LGPL, indicate your decision by
 * deletingthe provisions above and replace them with the notice and
 * other provisions required by the MPL or the GPL, as the case may be.
 * If you do not delete the provisions above, a recipient may use your
 * version of this file under any of the LGPL, the MPL or the GPL.
 */

#include "platform/image-decoders/jpeg/JPEGImageDecoder.h"

#include <memory>
#include "build/build_config.h"
#include "platform/instrumentation/PlatformInstrumentation.h"
#include "platform/wtf/PtrUtil.h"

namespace {

// JPEG only supports a denominator of 8.
const unsigned scaleDenominator = 8;

}  // namespace

namespace blink {

JPEGImageDecoder::JPEGImageDecoder(AlphaOption alpha_option,
                                   const ColorBehavior& color_behavior,
                                   size_t max_decoded_bytes)
    : ImageDecoder(alpha_option, color_behavior, max_decoded_bytes), codec_() {}

JPEGImageDecoder::~JPEGImageDecoder() {}

bool JPEGImageDecoder::SetSize(unsigned width, unsigned height) {
  if (!ImageDecoder::SetSize(width, height))
    return false;

  unsigned num = DesiredScaleNumerator();
  if (!num)
    return SetFailed();

  if (!codec_)
    return false;

  float scale = (float)num / (float)scaleDenominator;
  SkISize size = codec_->getScaledDimensions(scale);

  // Set output decoded size and orientation
  SetDecodedSize(size.width(), size.height());
  SetOrientation(ImageOrientation::FromEXIFValue((int)codec_->getOrigin()));

  // Set embedded colorspace for ICC profile
  if (!codec_->getInfo().colorSpace()->isSRGB())
    SetEmbeddedColorSpace(codec_->getInfo().refColorSpace());

  return true;
}

void JPEGImageDecoder::OnSetData(SegmentReader* data) {
  if (data) {
    if (!codec_) {
      SkCodec* codec = SkCodec::NewFromData(data->GetAsSkData());
      if (codec)
        codec_.reset(codec);
      else
        return;
    }
    SkImageInfo image_info = codec_->getInfo();
    SetSize(image_info.width(), image_info.height());
  }
}

void JPEGImageDecoder::SetDecodedSize(unsigned width, unsigned height) {
  decoded_size_ = IntSize(width, height);
}

bool JPEGImageDecoder::onQueryYUV8(SkYUVSizeInfo* size_info,
                                   SkYUVColorSpace* color_space) const {
  DCHECK(codec_);
  return codec_->queryYUV8(size_info, color_space);
}

unsigned JPEGImageDecoder::DesiredScaleNumerator() const {
  size_t original_bytes = Size().Width() * Size().Height() * 4;

  if (original_bytes <= max_decoded_bytes_)
    return scaleDenominator;

  // Downsample according to the maximum decoded size.
  unsigned scale_numerator = static_cast<unsigned>(floor(sqrt(
      // MSVC needs explicit parameter type for sqrt().
      static_cast<float>(max_decoded_bytes_ * scaleDenominator *
                         scaleDenominator / original_bytes))));

  return scale_numerator;
}

bool JPEGImageDecoder::CanDecodeToYUV() {
  // Calling IsSizeAvailable() ensures the decoder is created.
  return IsSizeAvailable();
}

bool JPEGImageDecoder::DecodeToYUV(const SkYUVSizeInfo& sizeInfo,
                                   void* planes[3]) {
  PlatformInstrumentation::WillDecodeImage("JPEG");
  SkCodec::Result result = codec_->getYUV8Planes(sizeInfo, planes);
  PlatformInstrumentation::DidDecodeImage();

  switch (result) {
    case SkCodec::kSuccess:
    case SkCodec::kIncompleteInput:
      return true;
    default:
      return false;
  }
}

void JPEGImageDecoder::Complete() {
  if (frame_buffer_cache_.IsEmpty())
    return;

  frame_buffer_cache_[0].SetHasAlpha(false);
  frame_buffer_cache_[0].SetStatus(ImageFrame::kFrameComplete);
}

inline bool IsComplete(const JPEGImageDecoder* decoder, bool only_size) {
  if (!only_size)
    return true;

  return decoder->FrameIsDecodedAtIndex(0);
}

bool JPEGImageDecoder::frameIsCompleteAtIndex(size_t index) const {
  if (!codec_)
    return false;

  if (index > 0)
    return false;

  if (frame_buffer_cache_.IsEmpty())
    return false;

  return frame_buffer_cache_[0].GetStatus() == ImageFrame::kFrameComplete;
}

void JPEGImageDecoder::Decode(bool only_size) {
  if (Failed())
    return;

  if (!codec_)
    return;

  if (!frame_buffer_cache_.size()) {
    // It is a fatal error if all data is received and the file is truncated.
    if (IsAllDataReceived())
      SetFailed();
    return;
  }

  SkImageInfo image_info = codec_->getInfo();
  SkCodec::Options options;
  ImageFrame& frame = frame_buffer_cache_[0];

  if (frame.GetStatus() == ImageFrame::kFrameEmpty) {
    frame.AllocatePixelData(decoded_size_.Width(), decoded_size_.Height(),
                            ColorSpaceForSkImages());
  }

  SkCodec::Result result =
      codec_->getPixels(image_info, frame.Bitmap().getPixels(),
                        frame.Bitmap().rowBytes(), &options, nullptr, nullptr);
  switch (result) {
    case SkCodec::kSuccess:
      frame.SetPixelsChanged(true);
      frame.SetStatus(ImageFrame::kFrameComplete);
      break;
    case SkCodec::kIncompleteInput:
      frame.SetPixelsChanged(true);
      frame.SetStatus(ImageFrame::kFramePartial);
      break;
    default:
      SetFailed();
      return;
  }
}

}  // namespace blink
