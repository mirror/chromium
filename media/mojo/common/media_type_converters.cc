// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/common/media_type_converters.h"

#include <stddef.h>
#include <stdint.h>

#include "base/memory/ptr_util.h"
#include "base/numerics/safe_conversions.h"
#include "media/base/audio_buffer.h"
#include "media/base/audio_bus.h"
#include "media/base/audio_decoder_config.h"
#include "media/base/cdm_key_information.h"
#include "media/base/data_buffer.h"
#include "media/base/decoder_buffer.h"
#include "media/base/decrypt_config.h"
#include "media/base/encryption_scheme.h"
#include "media/base/media_tracks.h"
#include "media/base/ranges.h"
#include "media/base/subsample_entry.h"
#include "mojo/public/cpp/system/buffer.h"

namespace mojo {

template <>
struct TypeConverter<media::mojom::PatternPtr,
                     media::EncryptionScheme::Pattern> {
  static media::mojom::PatternPtr Convert(
      const media::EncryptionScheme::Pattern& input);
};
template <>
struct TypeConverter<media::EncryptionScheme::Pattern,
                     media::mojom::PatternPtr> {
  static media::EncryptionScheme::Pattern Convert(
      const media::mojom::PatternPtr& input);
};

// static
media::mojom::DecryptConfigPtr
TypeConverter<media::mojom::DecryptConfigPtr, media::DecryptConfig>::Convert(
    const media::DecryptConfig& input) {
  media::mojom::DecryptConfigPtr mojo_decrypt_config(
      media::mojom::DecryptConfig::New());
  mojo_decrypt_config->key_id = input.key_id();
  mojo_decrypt_config->iv = input.iv();
  mojo_decrypt_config->subsamples = input.subsamples();

  return mojo_decrypt_config;
}

// static
std::unique_ptr<media::DecryptConfig>
TypeConverter<std::unique_ptr<media::DecryptConfig>,
              media::mojom::DecryptConfigPtr>::
    Convert(const media::mojom::DecryptConfigPtr& input) {
  return base::MakeUnique<media::DecryptConfig>(input->key_id, input->iv,
                                                input->subsamples);
}

// static
media::mojom::MediaTracksPtr
TypeConverter<media::mojom::MediaTracksPtr, media::MediaTracks>::Convert(
    const media::MediaTracks& input) {
  media::mojom::MediaTracksPtr mojo_media_tracks(
      media::mojom::MediaTracks::New());

  for (const auto& media_track : input.tracks()) {
    media::mojom::MediaTrackPtr mojo_media_track(
        media::mojom::MediaTrack::New());
    mojo_media_track->type = media_track->type();
    mojo_media_track->id = media_track->id();
    mojo_media_track->bytestream_track_id = media_track->bytestream_track_id();
    mojo_media_track->kind = media_track->kind();
    mojo_media_track->label = media_track->label();
    mojo_media_track->language = media_track->language();
    if (media_track->type() == media::MediaTrack::Audio)
      mojo_media_tracks
          ->audio_decoder_configs[media_track->bytestream_track_id()] =
          input.getAudioConfig(media_track->bytestream_track_id());
    else if (media_track->type() == media::MediaTrack::Video)
      mojo_media_tracks
          ->video_decoder_configs[media_track->bytestream_track_id()] =
          input.getVideoConfig(media_track->bytestream_track_id());

    mojo_media_tracks->tracks.push_back(std::move(mojo_media_track));
  }

  return mojo_media_tracks;
}

// static
std::unique_ptr<media::MediaTracks> TypeConverter<
    std::unique_ptr<media::MediaTracks>,
    media::mojom::MediaTracksPtr>::Convert(const media::mojom::MediaTracksPtr&
                                               input) {
  std::unique_ptr<media::MediaTracks> media_tracks(new media::MediaTracks());

  for (const auto& mojo_track : input->tracks) {
    media::MediaTrack* media_track = nullptr;
    if (mojo_track->type == media::MediaTrack::Type::Audio) {
      media_track = media_tracks->AddAudioTrack(
          input->audio_decoder_configs[mojo_track->bytestream_track_id],
          mojo_track->bytestream_track_id, mojo_track->kind, mojo_track->label,
          mojo_track->language);
    } else if (mojo_track->type == media::MediaTrack::Type::Video) {
      media_track = media_tracks->AddVideoTrack(
          input->video_decoder_configs[mojo_track->bytestream_track_id],
          mojo_track->bytestream_track_id, mojo_track->kind, mojo_track->label,
          mojo_track->language);
    }

    if (media_track)
      media_track->set_id(mojo_track->id);
  }
  return media_tracks;
}

// static
media::mojom::RangesTimeDeltaPtr TypeConverter<media::mojom::RangesTimeDeltaPtr,
                                               media::Ranges<base::TimeDelta>>::
    Convert(const media::Ranges<base::TimeDelta>& input) {
  media::mojom::RangesTimeDeltaPtr mojo_ranges_time_delta(
      media::mojom::RangesTimeDelta::New());

  for (size_t i = 0; i < input.size(); ++i) {
    media::mojom::TimeDeltaPairPtr mojo_time_delta_pair(
        media::mojom::TimeDeltaPair::New());
    mojo_time_delta_pair->start = input.start(i);
    mojo_time_delta_pair->end = input.end(i);

    mojo_ranges_time_delta->ranges.push_back(std::move(mojo_time_delta_pair));
  }

  return mojo_ranges_time_delta;
}

// static
media::Ranges<base::TimeDelta> TypeConverter<media::Ranges<base::TimeDelta>,
                                             media::mojom::RangesTimeDeltaPtr>::
    Convert(const media::mojom::RangesTimeDeltaPtr& input) {
  media::Ranges<base::TimeDelta> ranges_time_delta;

  for (const auto& mojo_time_delta_pair : input->ranges)
    ranges_time_delta.Add(mojo_time_delta_pair->start,
                          mojo_time_delta_pair->end);

  return ranges_time_delta;
}

// static
media::mojom::DataBufferPtr
TypeConverter<media::mojom::DataBufferPtr, scoped_refptr<media::DataBuffer>>::
    Convert(const scoped_refptr<media::DataBuffer>& input) {
  DCHECK(input);

  media::mojom::DataBufferPtr mojo_buffer(media::mojom::DataBuffer::New());
  if (input->end_of_stream()) {
    mojo_buffer->is_end_of_stream = true;
    return mojo_buffer;
  }

  mojo_buffer->is_end_of_stream = false;
  mojo_buffer->timestamp = input->timestamp();
  mojo_buffer->duration = input->duration();
  mojo_buffer->data_size = base::checked_cast<uint32_t>(input->data_size());

  // TODO(dalecurtis): We intentionally do not serialize the data section of
  // the DecoderBuffer here; this must instead be done by clients via their
  // own DataPipe.  See http://crbug.com/432960

  return mojo_buffer;
}

// static
scoped_refptr<media::DataBuffer>
TypeConverter<scoped_refptr<media::DataBuffer>, media::mojom::DataBufferPtr>::
    Convert(const media::mojom::DataBufferPtr& input) {
  if (input->is_end_of_stream)
    return media::DataBuffer::CreateEOSBuffer();

  scoped_refptr<media::DataBuffer> buffer(
      new media::DataBuffer(input->data_size));
  buffer->set_data_size(input->data_size);
  buffer->set_timestamp(input->timestamp);
  buffer->set_duration(input->duration);

  // TODO(dalecurtis): We intentionally do not deserialize the data section of
  // the DecoderBuffer here; this must instead be done by clients via their
  // own DataPipe.  See http://crbug.com/432960

  return buffer;
}

// static
media::mojom::DecoderBufferPtr
TypeConverter<media::mojom::DecoderBufferPtr,
              scoped_refptr<media::DecoderBuffer>>::
    Convert(const scoped_refptr<media::DecoderBuffer>& input) {
  DCHECK(input);

  media::mojom::DecoderBufferPtr mojo_buffer(
      media::mojom::DecoderBuffer::New());
  if (input->end_of_stream()) {
    mojo_buffer->is_end_of_stream = true;
    return mojo_buffer;
  }

  mojo_buffer->is_end_of_stream = false;
  mojo_buffer->timestamp = input->timestamp();
  mojo_buffer->duration = input->duration();
  mojo_buffer->is_key_frame = input->is_key_frame();
  mojo_buffer->data_size = base::checked_cast<uint32_t>(input->data_size());
  mojo_buffer->front_discard = input->discard_padding().first;
  mojo_buffer->back_discard = input->discard_padding().second;

  // Note: The side data is always small, so this copy is okay.
  if (input->side_data()) {
    DCHECK_GT(input->side_data_size(), 0u);
    mojo_buffer->side_data.assign(input->side_data(),
                                  input->side_data() + input->side_data_size());
  }

  if (input->decrypt_config()) {
    mojo_buffer->decrypt_config =
        media::mojom::DecryptConfig::From(*input->decrypt_config());
  }

  // TODO(dalecurtis): We intentionally do not serialize the data section of
  // the DecoderBuffer here; this must instead be done by clients via their
  // own DataPipe.  See http://crbug.com/432960

  return mojo_buffer;
}

// static
scoped_refptr<media::DecoderBuffer>
TypeConverter<scoped_refptr<media::DecoderBuffer>,
              media::mojom::DecoderBufferPtr>::
    Convert(const media::mojom::DecoderBufferPtr& input) {
  if (input->is_end_of_stream)
    return media::DecoderBuffer::CreateEOSBuffer();

  scoped_refptr<media::DecoderBuffer> buffer(
      new media::DecoderBuffer(input->data_size));

  if (!input->side_data.empty())
    buffer->CopySideDataFrom(input->side_data.data(), input->side_data.size());

  buffer->set_timestamp(input->timestamp);
  buffer->set_duration(input->duration);
  buffer->set_is_key_frame(input->is_key_frame);

  if (input->decrypt_config) {
    buffer->set_decrypt_config(
        input->decrypt_config.To<std::unique_ptr<media::DecryptConfig>>());
  }

  media::DecoderBuffer::DiscardPadding discard_padding(input->front_discard,
                                                       input->back_discard);
  buffer->set_discard_padding(discard_padding);

  // TODO(dalecurtis): We intentionally do not deserialize the data section of
  // the DecoderBuffer here; this must instead be done by clients via their
  // own DataPipe.  See http://crbug.com/432960

  return buffer;
}

// static
media::mojom::AudioDecoderConfigPtr
TypeConverter<media::mojom::AudioDecoderConfigPtr, media::AudioDecoderConfig>::
    Convert(const media::AudioDecoderConfig& input) {
  media::mojom::AudioDecoderConfigPtr config(
      media::mojom::AudioDecoderConfig::New());
  config->codec = input.codec();
  config->sample_format = input.sample_format();
  config->channel_layout = input.channel_layout();
  config->samples_per_second = input.samples_per_second();
  config->extra_data = input.extra_data();
  config->seek_preroll = input.seek_preroll();
  config->codec_delay = input.codec_delay();
  config->encryption_scheme = input.encryption_scheme();
  return config;
}

// static
media::AudioDecoderConfig
TypeConverter<media::AudioDecoderConfig, media::mojom::AudioDecoderConfigPtr>::
    Convert(const media::mojom::AudioDecoderConfigPtr& input) {
  media::AudioDecoderConfig config;
  config.Initialize(input->codec, input->sample_format, input->channel_layout,
                    input->samples_per_second, input->extra_data,
                    input->encryption_scheme, input->seek_preroll,
                    input->codec_delay);
  return config;
}

// static
media::mojom::CdmKeyInformationPtr TypeConverter<
    media::mojom::CdmKeyInformationPtr,
    media::CdmKeyInformation>::Convert(const media::CdmKeyInformation& input) {
  media::mojom::CdmKeyInformationPtr info(
      media::mojom::CdmKeyInformation::New());
  info->key_id = input.key_id;
  info->status = input.status;
  info->system_code = input.system_code;
  return info;
}

// static
std::unique_ptr<media::CdmKeyInformation>
TypeConverter<std::unique_ptr<media::CdmKeyInformation>,
              media::mojom::CdmKeyInformationPtr>::
    Convert(const media::mojom::CdmKeyInformationPtr& input) {
  return base::MakeUnique<media::CdmKeyInformation>(
      input->key_id, input->status, input->system_code);
}

// static
media::mojom::AudioBufferPtr
TypeConverter<media::mojom::AudioBufferPtr, scoped_refptr<media::AudioBuffer>>::
    Convert(const scoped_refptr<media::AudioBuffer>& input) {
  media::mojom::AudioBufferPtr buffer(media::mojom::AudioBuffer::New());
  buffer->sample_format = input->sample_format_;
  buffer->channel_layout = input->channel_layout();
  buffer->channel_count = input->channel_count();
  buffer->sample_rate = input->sample_rate();
  buffer->frame_count = input->frame_count();
  buffer->end_of_stream = input->end_of_stream();
  buffer->timestamp = input->timestamp();

  if (input->data_) {
    DCHECK_GT(input->data_size(), 0u);
    buffer->data.assign(input->data_.get(),
                        input->data_.get() + input->data_size_);
  }

  return buffer;
}

// static
scoped_refptr<media::AudioBuffer>
TypeConverter<scoped_refptr<media::AudioBuffer>, media::mojom::AudioBufferPtr>::
    Convert(const media::mojom::AudioBufferPtr& input) {
  if (input->end_of_stream)
    return media::AudioBuffer::CreateEOSBuffer();

  if (input->frame_count <= 0 ||
      static_cast<size_t>(input->sample_format) > media::kSampleFormatMax ||
      static_cast<size_t>(input->channel_layout) > media::CHANNEL_LAYOUT_MAX ||
      ChannelLayoutToChannelCount(input->channel_layout) !=
          input->channel_count) {
    LOG(ERROR) << "Receive an invalid audio buffer, replace it with EOS.";
    return media::AudioBuffer::CreateEOSBuffer();
  }

  if (IsBitstream(input->sample_format)) {
    uint8_t* data = input->data.data();
    return media::AudioBuffer::CopyBitstreamFrom(
        input->sample_format, input->channel_layout, input->channel_count,
        input->sample_rate, input->frame_count, &data, input->data.size(),
        input->timestamp);
  }

  // Setup channel pointers.  AudioBuffer::CopyFrom() will only use the first
  // one in the case of interleaved data.
  std::vector<const uint8_t*> channel_ptrs(input->channel_count, nullptr);
  const size_t size_per_channel = input->data.size() / input->channel_count;
  DCHECK_EQ(0u, input->data.size() % input->channel_count);
  for (int i = 0; i < input->channel_count; ++i)
    channel_ptrs[i] = input->data.data() + i * size_per_channel;

  return media::AudioBuffer::CopyFrom(
      input->sample_format, input->channel_layout, input->channel_count,
      input->sample_rate, input->frame_count, &channel_ptrs[0],
      input->timestamp);
}

// static
media::mojom::AudioBusPtr
TypeConverter<media::mojom::AudioBusPtr, media::AudioBus>::Convert(
    const media::AudioBus& input) {
  media::mojom::AudioBusPtr buffer(media::mojom::AudioBus::New());
  buffer->nb_channels = input.channels();
  buffer->nb_frames = input.frames();

  if (input.data_) {
    DCHECK_GT(input.data_size_, 0u);
    buffer->data.assign(input.data_.get(),
                        input.data_.get() + input.data_size_);
  }

  return buffer;
}

}  // namespace mojo
