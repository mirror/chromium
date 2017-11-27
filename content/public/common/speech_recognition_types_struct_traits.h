// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_SPEECH_RECOGNITION_TYPES_STRUCT_TRAITS_H_
#define CONTENT_PUBLIC_COMMON_SPEECH_RECOGNITION_TYPES_STRUCT_TRAITS_H_

#include "content/public/common/speech_recognition_error.h"
#include "content/public/common/speech_recognition_result.h"
#include "content/public/common/speech_recognition_types.mojom-shared.h"
#include "mojo/public/cpp/bindings/enum_traits.h"

namespace mojo {

template <>
struct StructTraits<content::mojom::SpeechRecognitionHypothesisDataView,
                    content::SpeechRecognitionHypothesis> {
  static const base::string16& utterance(
      const content::SpeechRecognitionHypothesis& r) {
    return r.utterance;
  }
  static double confidence(const content::SpeechRecognitionHypothesis& r) {
    return r.confidence;
  }
  static bool Read(content::mojom::SpeechRecognitionHypothesisDataView data,
                   content::SpeechRecognitionHypothesis* out) {
    if (!data.ReadUtterance(&out->utterance))
      return false;

    out->confidence = data.confidence();
    return true;
  }
};

template <>
struct StructTraits<content::mojom::SpeechRecognitionResultDataView,
                    content::SpeechRecognitionResult> {
  static const std::vector<content::SpeechRecognitionHypothesis>& hypotheses(
      const content::SpeechRecognitionResult& r) {
    return r.hypotheses;
  }
  static bool is_provisional(const content::SpeechRecognitionResult& r) {
    return r.is_provisional;
  }
  static bool Read(content::mojom::SpeechRecognitionResultDataView data,
                   content::SpeechRecognitionResult* out) {
    if (!data.ReadHypotheses(&out->hypotheses))
      return false;

    out->is_provisional = data.is_provisional();
    return true;
  }
};

template <>
struct EnumTraits<content::mojom::SpeechRecognitionErrorCode,
                  content::SpeechRecognitionErrorCode> {
  static content::mojom::SpeechRecognitionErrorCode ToMojom(
      content::SpeechRecognitionErrorCode error_code) {
    switch (error_code) {
      case content::SPEECH_RECOGNITION_ERROR_NONE:
        return content::mojom::SpeechRecognitionErrorCode::kNone;
      case content::SPEECH_RECOGNITION_ERROR_NO_SPEECH:
        return content::mojom::SpeechRecognitionErrorCode::kNoSpeech;
      case content::SPEECH_RECOGNITION_ERROR_ABORTED:
        return content::mojom::SpeechRecognitionErrorCode::kAborted;
      case content::SPEECH_RECOGNITION_ERROR_AUDIO_CAPTURE:
        return content::mojom::SpeechRecognitionErrorCode::kAudioCapture;
      case content::SPEECH_RECOGNITION_ERROR_NETWORK:
        return content::mojom::SpeechRecognitionErrorCode::kNetwork;
      case content::SPEECH_RECOGNITION_ERROR_NOT_ALLOWED:
        return content::mojom::SpeechRecognitionErrorCode::kNotAllowed;
      case content::SPEECH_RECOGNITION_ERROR_SERVICE_NOT_ALLOWED:
        return content::mojom::SpeechRecognitionErrorCode::kServiceNotAllowed;
      case content::SPEECH_RECOGNITION_ERROR_BAD_GRAMMAR:
        return content::mojom::SpeechRecognitionErrorCode::kBadGrammar;
      case content::SPEECH_RECOGNITION_ERROR_LANGUAGE_NOT_SUPPORTED:
        return content::mojom::SpeechRecognitionErrorCode::
            kLanguageNotSupported;
      case content::SPEECH_RECOGNITION_ERROR_NO_MATCH:
        return content::mojom::SpeechRecognitionErrorCode::kNoMatch;
    }
    NOTREACHED();
    return content::mojom::SpeechRecognitionErrorCode::kNoMatch;
  }

  static bool FromMojom(content::mojom::SpeechRecognitionErrorCode error_code,
                        content::SpeechRecognitionErrorCode* out) {
    switch (error_code) {
      case content::mojom::SpeechRecognitionErrorCode::kNone:
        *out = content::SPEECH_RECOGNITION_ERROR_NONE;
        return true;
      case content::mojom::SpeechRecognitionErrorCode::kNoSpeech:
        *out = content::SPEECH_RECOGNITION_ERROR_NO_SPEECH;
        return true;
      case content::mojom::SpeechRecognitionErrorCode::kAborted:
        *out = content::SPEECH_RECOGNITION_ERROR_ABORTED;
        return true;
      case content::mojom::SpeechRecognitionErrorCode::kAudioCapture:
        *out = content::SPEECH_RECOGNITION_ERROR_AUDIO_CAPTURE;
        return true;
      case content::mojom::SpeechRecognitionErrorCode::kNetwork:
        *out = content::SPEECH_RECOGNITION_ERROR_NETWORK;
        return true;
      case content::mojom::SpeechRecognitionErrorCode::kNotAllowed:
        *out = content::SPEECH_RECOGNITION_ERROR_NOT_ALLOWED;
        return true;
      case content::mojom::SpeechRecognitionErrorCode::kServiceNotAllowed:
        *out = content::SPEECH_RECOGNITION_ERROR_SERVICE_NOT_ALLOWED;
        return true;
      case content::mojom::SpeechRecognitionErrorCode::kBadGrammar:
        *out = content::SPEECH_RECOGNITION_ERROR_BAD_GRAMMAR;
        return true;
      case content::mojom::SpeechRecognitionErrorCode::kLanguageNotSupported:
        *out = content::SPEECH_RECOGNITION_ERROR_LANGUAGE_NOT_SUPPORTED;
        return true;
      case content::mojom::SpeechRecognitionErrorCode::kNoMatch:
        *out = content::SPEECH_RECOGNITION_ERROR_NO_MATCH;
        return true;
    }
    NOTREACHED();
    return false;
  }
};

template <>
struct EnumTraits<content::mojom::SpeechAudioErrorDetails,
                  content::SpeechAudioErrorDetails> {
  static content::mojom::SpeechAudioErrorDetails ToMojom(
      content::SpeechAudioErrorDetails error_details) {
    switch (error_details) {
      case content::SPEECH_AUDIO_ERROR_DETAILS_NONE:
        return content::mojom::SpeechAudioErrorDetails::kNone;
      case content::SPEECH_AUDIO_ERROR_DETAILS_NO_MIC:
        return content::mojom::SpeechAudioErrorDetails::kNoMic;
    }
    NOTREACHED();
    return content::mojom::SpeechAudioErrorDetails::kNone;
  }

  static bool FromMojom(content::mojom::SpeechAudioErrorDetails error_details,
                        content::SpeechAudioErrorDetails* out) {
    switch (error_details) {
      case content::mojom::SpeechAudioErrorDetails::kNone:
        *out = content::SPEECH_AUDIO_ERROR_DETAILS_NONE;
        return true;
      case content::mojom::SpeechAudioErrorDetails::kNoMic:
        *out = content::SPEECH_AUDIO_ERROR_DETAILS_NO_MIC;
        return true;
    }
    NOTREACHED();
    return false;
  }
};

template <>
struct StructTraits<content::mojom::SpeechRecognitionErrorDataView,
                    content::SpeechRecognitionError> {
  static content::SpeechRecognitionErrorCode code(
      const content::SpeechRecognitionError& r) {
    return r.code;
  }
  static content::SpeechAudioErrorDetails details(
      const content::SpeechRecognitionError& r) {
    return r.details;
  }
  static bool Read(content::mojom::SpeechRecognitionErrorDataView data,
                   content::SpeechRecognitionError* out) {
    if (!data.ReadCode(&out->code))
      return false;

    if (!data.ReadDetails(&out->details))
      return false;

    return true;
  }
};

}  // namespace mojo

#endif  // CONTENT_PUBLIC_COMMON_SPEECH_RECOGNITION_TYPES_STRUCT_TRAITS_H_
