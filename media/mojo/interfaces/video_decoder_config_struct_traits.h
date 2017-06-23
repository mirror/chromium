// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_INTERFACES_VIDEO_DECODER_CONFIG_STRUCT_TRAITS_H_
#define MEDIA_MOJO_INTERFACES_VIDEO_DECODER_CONFIG_STRUCT_TRAITS_H_

#include "media/base/video_decoder_config.h"
#include "media/mojo/interfaces/media_types.mojom.h"

namespace mojo {

template <>
struct StructTraits<media::mojom::VideoDecoderConfigDataView,
                    media::VideoDecoderConfig> {
  static media::VideoCodec codec(const media::VideoDecoderConfig& input) {
    return input.codec();
  }

  static media::VideoCodecProfile profile(
      const media::VideoDecoderConfig& input) {
    return input.profile();
  }

  static media::VideoPixelFormat format(
      const media::VideoDecoderConfig& input) {
    return input.format();
  }

  static media::ColorSpace color_space(const media::VideoDecoderConfig& input) {
    return input.color_space();
  }

  static gfx::Size coded_size(const media::VideoDecoderConfig& input) {
    return input.coded_size();
  }

  static gfx::Rect visible_rect(const media::VideoDecoderConfig& input) {
    return input.visible_rect();
  }

  static gfx::Size natural_size(const media::VideoDecoderConfig& input) {
    return input.natural_size();
  }

  static std::vector<uint8_t> extra_data(
      const media::VideoDecoderConfig& input) {
    return input.extra_data();
  }

  static media::EncryptionScheme encryption_scheme(
      const media::VideoDecoderConfig& input) {
    return input.encryption_scheme();
  }

  static media::VideoColorSpace color_space_info(
      const media::VideoDecoderConfig& input) {
    return input.color_space_info();
  }

  static base::Optional<media::HDRMetadata> hdr_metadata(
      const media::VideoDecoderConfig& input) {
    return input.hdr_metadata();
  }

  static bool Read(media::mojom::VideoDecoderConfigDataView input,
                   media::VideoDecoderConfig* output) {
    gfx::Size coded_size;
    if (!input.ReadCodedSize(&coded_size))
      return false;

    gfx::Rect visible_rect;
    if (!input.ReadVisibleRect(&visible_rect))
      return false;

    gfx::Size natural_size;
    if (!input.ReadNaturalSize(&natural_size))
      return false;

    std::vector<uint8_t> extra_data;
    if (!input.ReadExtraData(&extra_data))
      return false;

    media::EncryptionScheme encryption_scheme;
    if (!input.ReadEncryptionScheme(&encryption_scheme))
      return false;

    media::VideoColorSpace color_space_info;
    if (!input.ReadColorSpaceInfo(&color_space_info))
      return false;

    base::Optional<media::HDRMetadata> hdr_metadata;
    if (!input.ReadHdrMetadata(&hdr_metadata))
      return false;

    output->Initialize(static_cast<media::VideoCodec>(input.codec()),
                       static_cast<media::VideoCodecProfile>(input.profile()),
                       static_cast<media::VideoPixelFormat>(input.format()),
                       static_cast<media::ColorSpace>(input.color_space()),
                       coded_size, visible_rect, natural_size, extra_data,
                       encryption_scheme);

    output->set_color_space_info(color_space_info);

    if (hdr_metadata)
      output->set_hdr_metadata(hdr_metadata.value());

    return true;
  }
};

}  // namespace mojo

#endif  // MEDIA_MOJO_INTERFACES_VIDEO_DECODER_CONFIG_STRUCT_TRAITS_H_
