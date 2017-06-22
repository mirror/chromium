// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_INTERFACES_AUDIO_DECODER_CONFIG_STRUCT_TRAITS_H_
#define MEDIA_MOJO_INTERFACES_AUDIO_DECODER_CONFIG_STRUCT_TRAITS_H_

#include "media/base/audio_decoder_config.h"
#include "media/mojo/interfaces/media_types.mojom.h"

namespace mojo {

template <>
struct StructTraits<media::mojom::AudioDecoderConfigDataView,
                    media::AudioDecoderConfig> {
  static media::AudioCodec codec(const media::AudioDecoderConfig& input) {
    return input.codec();
  }

  static media::SampleFormat sample_format(
      const media::AudioDecoderConfig& input) {
    return input.sample_format();
  }

  static media::ChannelLayout channel_layout(
      const media::AudioDecoderConfig& input) {
    return input.channel_layout();
  }

  static int samples_per_second(const media::AudioDecoderConfig& input) {
    return input.samples_per_second();
  }

  static std::vector<uint8_t> extra_data(
      const media::AudioDecoderConfig& input) {
    return input.extra_data();
  }

  static base::TimeDelta seek_preroll(const media::AudioDecoderConfig& input) {
    return input.seek_preroll();
  }

  static int codec_delay(const media::AudioDecoderConfig& input) {
    return input.codec();
  }

  static media::EncryptionScheme encryption_scheme(
      const media::AudioDecoderConfig& input) {
    return input.encryption_scheme();
  }

  static bool Read(media::mojom::AudioDecoderConfigDataView input,
                   media::AudioDecoderConfig* output) {
    std::vector<uint8_t> extra_data;
    if (!input.ReadExtraData(&extra_data))
      return false;

    media::EncryptionScheme encryption_scheme;
    if (!input.ReadEncryptionScheme(&encryption_scheme))
      return false;

    base::TimeDelta seek_preroll;
    if (!input.ReadSeekPreroll(&seek_preroll))
      return false;

    output->Initialize(
        static_cast<media::AudioCodec>(input.codec()),
        static_cast<media::SampleFormat>(input.sample_format()),
        static_cast<media::ChannelLayout>(input.channel_layout()),
        input.samples_per_second(), extra_data, encryption_scheme, seek_preroll,
        input.codec_delay());

    return true;
  }
};

}  // namespace mojo

#endif  // MEDIA_MOJO_INTERFACES_AUDIO_DECODER_CONFIG_STRUCT_TRAITS_H_
