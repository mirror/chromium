// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_REMOTING_PROTO_ENUM_UTILS_H_
#define MEDIA_REMOTING_PROTO_ENUM_UTILS_H_

#include "base/optional.h"
#include "media/base/audio_codecs.h"
#include "media/base/buffering_state.h"
#include "media/base/cdm_key_information.h"
#include "media/base/cdm_promise.h"
#include "media/base/channel_layout.h"
#include "media/base/content_decryption_module.h"
#include "media/base/demuxer_stream.h"
#include "media/base/encryption_scheme.h"
#include "media/base/sample_format.h"
#include "media/base/video_codecs.h"
#include "media/base/video_types.h"
#include "media/remoting/rpc.pb.h"

namespace media {
namespace remoting {

// The following functions map between the enum values in media/base modules and
// the equivalents in the media/remoting protobuf classes. The purpose of these
// converters is to decouple the media/base modules from the media/remoting
// modules while maintaining compile-time checks to ensure that there are always
// valid, backwards-compatible mappings between the two.

EncryptionScheme::CipherMode ToMediaEncryptionSchemeCipherMode(
    pb::EncryptionScheme::CipherMode value);
pb::EncryptionScheme::CipherMode ToProtoEncryptionSchemeCipherMode(
    EncryptionScheme::CipherMode value);

AudioCodec ToMediaAudioCodec(pb::AudioDecoderConfig::Codec value);
pb::AudioDecoderConfig::Codec ToProtoAudioDecoderConfigCodec(AudioCodec value);

SampleFormat ToMediaSampleFormat(pb::AudioDecoderConfig::SampleFormat value);
pb::AudioDecoderConfig::SampleFormat ToProtoAudioDecoderConfigSampleFormat(
    SampleFormat value);

ChannelLayout ToMediaChannelLayout(pb::AudioDecoderConfig::ChannelLayout value);
pb::AudioDecoderConfig::ChannelLayout ToProtoAudioDecoderConfigChannelLayout(
    ChannelLayout value);

VideoCodec ToMediaVideoCodec(pb::VideoDecoderConfig::Codec value);
pb::VideoDecoderConfig::Codec ToProtoVideoDecoderConfigCodec(VideoCodec value);

VideoCodecProfile ToMediaVideoCodecProfile(
    pb::VideoDecoderConfig::Profile value);
pb::VideoDecoderConfig::Profile ToProtoVideoDecoderConfigProfile(
    VideoCodecProfile value);

VideoPixelFormat ToMediaVideoPixelFormat(pb::VideoDecoderConfig::Format value);
pb::VideoDecoderConfig::Format ToProtoVideoDecoderConfigFormat(
    VideoPixelFormat value);

ColorSpace ToMediaColorSpace(pb::VideoDecoderConfig::ColorSpace value);
pb::VideoDecoderConfig::ColorSpace ToProtoVideoDecoderConfigColorSpace(
    ColorSpace value);

BufferingState ToMediaBufferingState(
    pb::RendererClientOnBufferingStateChange::State value);
pb::RendererClientOnBufferingStateChange::State ToProtoMediaBufferingState(
    BufferingState value);

CdmKeyInformation::KeyStatus ToMediaCdmKeyInformationKeyStatus(
    pb::CdmKeyInformation::KeyStatus value);
pb::CdmKeyInformation::KeyStatus ToProtoCdmKeyInformation(
    CdmKeyInformation::KeyStatus value);

CdmPromise::Exception ToCdmPromiseException(pb::CdmException value);
pb::CdmException ToProtoCdmException(CdmPromise::Exception value);

CdmMessageType ToMediaCdmMessageType(pb::CdmMessageType value);
pb::CdmMessageType ToProtoCdmMessageType(CdmMessageType value);

CdmSessionType ToCdmSessionType(pb::CdmSessionType value);
pb::CdmSessionType ToProtoCdmSessionType(CdmSessionType value);

EmeInitDataType ToMediaEmeInitDataType(
    pb::CdmCreateSessionAndGenerateRequest::EmeInitDataType value);
pb::CdmCreateSessionAndGenerateRequest::EmeInitDataType
ToProtoMediaEmeInitDataType(EmeInitDataType value);

DemuxerStream::Status ToDemuxerStreamStatus(
    pb::DemuxerStreamReadUntilCallback::Status value);
pb::DemuxerStreamReadUntilCallback::Status ToProtoDemuxerStreamStatus(
    DemuxerStream::Status value);

}  // namespace remoting
}  // namespace media

#endif  // MEDIA_REMOTING_PROTO_ENUM_UTILS_H_
