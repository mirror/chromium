// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_EME_CONSTANTS_H_
#define MEDIA_BASE_EME_CONSTANTS_H_

#include <stdint.h>

namespace media {

// Defines bitmask values that specify registered initialization data types used
// in Encrypted Media Extensions (EME).
// The mask values are stored in a SupportedInitDataTypes.
enum EmeInitDataType {
  EME_INIT_DATA_TYPE_NONE = 0,
  EME_INIT_DATA_TYPE_WEBM = 1 << 0,
#if defined(USE_PROPRIETARY_CODECS)
  EME_INIT_DATA_TYPE_CENC = 1 << 1,
#endif  // defined(USE_PROPRIETARY_CODECS)
  EME_INIT_DATA_TYPE_KEYIDS = 1 << 2,
};

// Defines bitmask values that specify codecs used in Encrypted Media Extension
// (EME). Each value represents a codec within a specific container.
// The mask values are stored in a SupportedCodecs.
enum EmeCodec {
  // *_ALL values should only be used for masking, do not use them to specify
  // codec support because they may be extended to include more codecs.
  EME_CODEC_NONE = 0,
  EME_CODEC_WEBM_OPUS = 1 << 0,
  EME_CODEC_WEBM_VORBIS = 1 << 1,
  EME_CODEC_WEBM_AUDIO_ALL = EME_CODEC_WEBM_OPUS | EME_CODEC_WEBM_VORBIS,
  EME_CODEC_WEBM_VP8 = 1 << 2,
  EME_CODEC_WEBM_VP9 = 1 << 3,
  EME_CODEC_WEBM_VIDEO_ALL = (EME_CODEC_WEBM_VP8 | EME_CODEC_WEBM_VP9),
  EME_CODEC_WEBM_ALL = (EME_CODEC_WEBM_AUDIO_ALL | EME_CODEC_WEBM_VIDEO_ALL),
#if defined(USE_PROPRIETARY_CODECS)
  EME_CODEC_MP4_AAC = 1 << 4,
  EME_CODEC_MP4_AUDIO_ALL = EME_CODEC_MP4_AAC,
  EME_CODEC_MP4_AVC1 = 1 << 5,
  EME_CODEC_MP4_VIDEO_ALL = EME_CODEC_MP4_AVC1,
  EME_CODEC_MP4_ALL = (EME_CODEC_MP4_AUDIO_ALL | EME_CODEC_MP4_VIDEO_ALL),
  EME_CODEC_AUDIO_ALL = (EME_CODEC_WEBM_AUDIO_ALL | EME_CODEC_MP4_AUDIO_ALL),
  EME_CODEC_VIDEO_ALL = (EME_CODEC_WEBM_VIDEO_ALL | EME_CODEC_WEBM_VIDEO_ALL),
  EME_CODEC_ALL = (EME_CODEC_WEBM_ALL | EME_CODEC_MP4_ALL),
#else
  EME_CODEC_AUDIO_ALL = EME_CODEC_WEBM_AUDIO_ALL,
  EME_CODEC_VIDEO_ALL = EME_CODEC_WEBM_VIDEO_ALL,
  EME_CODEC_ALL = EME_CODEC_WEBM_ALL,
#endif  // defined(USE_PROPRIETARY_CODECS)
};

typedef uint32_t SupportedInitDataTypes;
typedef uint32_t SupportedCodecs;

enum EmeSessionTypeSupport {
  // Invalid default value.
  EME_SESSION_TYPE_INVALID,
  // The session type is not supported.
  EME_SESSION_TYPE_NOT_SUPPORTED,
  // The session type is supported if a distinctive identifier is available.
  EME_SESSION_TYPE_SUPPORTED_WITH_IDENTIFIER,
  // The session type is always supported.
  EME_SESSION_TYPE_SUPPORTED,
};

// Used to declare support for distinctive identifier and persistent state.
enum EmeFeatureSupport {
  // Invalid default value.
  EME_FEATURE_INVALID,
  // Access to the feature is not supported at all.
  EME_FEATURE_NOT_SUPPORTED,
  // Access to the feature may be requested if a distinctive identifier is
  // available. (This is the correct choice for declaring support for a
  // requestable distinctive identifier.)
  EME_FEATURE_REQUESTABLE_WITH_IDENTIFIER,
  // Access to the feature may be requested.
  EME_FEATURE_REQUESTABLE,
  // Access to the feature cannot be blocked.
  EME_FEATURE_ALWAYS_ENABLED,
};

// Used to query support for distinctive identifier and persistent state.
enum EmeFeatureRequirement {
  EME_FEATURE_NOT_ALLOWED,
  EME_FEATURE_OPTIONAL,
  EME_FEATURE_REQUIRED,
};

enum class EmeMediaType {
  AUDIO,
  VIDEO,
};

// Robustness values understood by KeySystems.
// Note: key_systems.cc expects this ordering in GetRobustnessConfigRule(),
// make sure to correct that code if this list changes.
enum class EmeRobustness {
  INVALID,
  EMPTY,
  SW_SECURE_CRYPTO,
  SW_SECURE_DECODE,
  HW_SECURE_CRYPTO,
  HW_SECURE_DECODE,
  HW_SECURE_ALL,
};

// Configuration rules indicate the configuration state required to support a
// configuration option (note: a configuration option may be disallowing a
// feature). Configuration rules are used to answer queries about distinctive
// identifier, persistent state, and robustness requirements, as well as to
// describe support for different session types.
//
// If in the future there are reasons to request user permission other than
// access to a distinctive identifier, then additional rules should be added.
// Rules are implemented in ConfigState and are otherwise opaque.
enum class EmeConfigRule {
  // The configuration option is not supported.
  NOT_SUPPORTED,
  // The configuration option is supported if a distinctive identifier is
  // available.
  IDENTIFIER_REQUIRED,
  // The configuration option is supported, but the user experience may be
  // improved if a distinctive identifier is available.
  IDENTIFIER_RECOMMENDED,
  // The configuration option is supported without conditions.
  SUPPORTED,
};

}  // namespace media

#endif  // MEDIA_BASE_EME_CONSTANTS_H_
