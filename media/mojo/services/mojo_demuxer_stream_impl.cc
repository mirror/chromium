// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/mojo_demuxer_stream_impl.h"

#include "base/bind.h"
#include "base/macros.h"
#include "media/base/audio_decoder_config.h"
#include "media/base/decoder_buffer.h"
#include "media/base/video_decoder_config.h"
#include "media/mojo/interfaces/demuxer_stream.mojom.h"
#include "media/mojo/services/media_type_converters.h"
#include "mojo/public/cpp/bindings/interface_impl.h"
#include "mojo/public/cpp/system/data_pipe.h"

namespace media {

MojoDemuxerStreamImpl::MojoDemuxerStreamImpl(media::DemuxerStream* stream)
    : stream_(stream), weak_factory_(this) {
}

MojoDemuxerStreamImpl::~MojoDemuxerStreamImpl() {
}

void MojoDemuxerStreamImpl::Read(const mojo::Callback<
    void(mojo::DemuxerStream::Status, mojo::MediaDecoderBufferPtr)>& callback) {
  stream_->Read(base::Bind(&MojoDemuxerStreamImpl::OnBufferReady,
                           weak_factory_.GetWeakPtr(),
                           callback));
}

void MojoDemuxerStreamImpl::OnBufferReady(
    const BufferReadyCB& callback,
    media::DemuxerStream::Status status,
    const scoped_refptr<media::DecoderBuffer>& buffer) {
  if (status == media::DemuxerStream::kConfigChanged) {
    // Send the config change so our client can read it once it parses the
    // Status obtained via Run() below.
    if (stream_->type() == media::DemuxerStream::AUDIO) {
      client()->OnAudioDecoderConfigChanged(
          mojo::AudioDecoderConfig::From(stream_->audio_decoder_config()));
    } else if (stream_->type() == media::DemuxerStream::VIDEO) {
      client()->OnVideoDecoderConfigChanged(
          mojo::VideoDecoderConfig::From(stream_->video_decoder_config()));
    }
  }

  // Serialize the data section of the DecoderBuffer into our pipe.
  uint32_t num_bytes = buffer->data_size();
  CHECK_EQ(WriteDataRaw(stream_pipe_.get(), buffer->data(), &num_bytes,
                        MOJO_READ_DATA_FLAG_ALL_OR_NONE),
           MOJO_RESULT_OK);
  CHECK_EQ(num_bytes, static_cast<uint32_t>(buffer->data_size()));

  // TODO(dalecurtis): Once we can write framed data to the DataPipe, fill via
  // the producer handle and then read more to keep the pipe full.  Waiting for
  // space can be accomplished using an AsyncWaiter.
  callback.Run(static_cast<mojo::DemuxerStream::Status>(status),
               mojo::MediaDecoderBuffer::From(buffer));
}

void MojoDemuxerStreamImpl::DidConnect() {
  // This is called when our DemuxerStreamClient has connected itself and is
  // ready to receive messages.  Send an initial config and notify it that
  // we are now ready for business.
  if (stream_->type() == media::DemuxerStream::AUDIO) {
    client()->OnAudioDecoderConfigChanged(
        mojo::AudioDecoderConfig::From(stream_->audio_decoder_config()));
  } else if (stream_->type() == media::DemuxerStream::VIDEO) {
    client()->OnVideoDecoderConfigChanged(
        mojo::VideoDecoderConfig::From(stream_->video_decoder_config()));
  }

  MojoCreateDataPipeOptions options;
  options.struct_size = sizeof(MojoCreateDataPipeOptions);
  options.flags = MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_NONE;
  options.element_num_bytes = 1;

  // Allocate DataPipe sizes based on content type to reduce overhead.  If this
  // is still too burdensome we can adjust for sample rate or resolution.
  if (stream_->type() == media::DemuxerStream::VIDEO) {
    // Video can get quite large; at 4K, VP9 delivers packets which are ~1MB in
    // size; so allow for 50% headroom.
    options.capacity_num_bytes = 1.5 * (1024 * 1024);
  } else {
    // Other types don't require a lot of room, so use a smaller pipe.
    options.capacity_num_bytes = 512 * 1024;
  }

  mojo::DataPipe data_pipe(options);
  stream_pipe_ = data_pipe.producer_handle.Pass();
  client()->OnStreamReady(data_pipe.consumer_handle.Pass());
}

}  // namespace media
