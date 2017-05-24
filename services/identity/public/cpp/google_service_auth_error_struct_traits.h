// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_IDENTITY_PUBLIC_CPP_GOOGLE_SERVICE_AUTH_ERROR_STRUCT_TRAITS_H_
#define SERVICES_IDENTITY_PUBLIC_CPP_GOOGLE_SERVICE_AUTH_ERROR_STRUCT_TRAITS_H_

#include <string>

#include "google_apis/gaia/google_service_auth_error.h"
#include "services/identity/public/interfaces/google_service_auth_error.mojom.h"

namespace mojo {

template <>
struct StructTraits<identity::mojom::GoogleServiceAuthError::DataView,
                    ::GoogleServiceAuthError> {
  static int state(const ::GoogleServiceAuthError& r) { return r.state(); }
  static const ::GoogleServiceAuthError::Captcha& captcha(
      const ::GoogleServiceAuthError& r) {
    return r.captcha();
  }
  static const ::GoogleServiceAuthError::SecondFactor& second_factor(
      const ::GoogleServiceAuthError& r) {
    return r.second_factor();
  }
  static int network_error(const ::GoogleServiceAuthError& r) {
    return r.network_error();
  }

  static std::string error_message(const ::GoogleServiceAuthError& r) {
    return r.error_message();
  }

  static bool Read(identity::mojom::GoogleServiceAuthErrorDataView data,
                   ::GoogleServiceAuthError* out) {
    int state = data.state();
    ::GoogleServiceAuthError::Captcha captcha;
    ::GoogleServiceAuthError::SecondFactor second_factor;
    std::string error_message;
    if (state < 0 || state > ::GoogleServiceAuthError::State::NUM_STATES ||
        !data.ReadCaptcha(&captcha) || !data.ReadSecondFactor(&second_factor) ||
        !data.ReadErrorMessage(&error_message)) {
      return false;
    }

    out->state_ = ::GoogleServiceAuthError::State(state);
    out->captcha_ = captcha;
    out->second_factor_ = second_factor;
    out->error_message_ = error_message;
    out->network_error_ = data.network_error();

    return true;
  }
};

template <>
struct StructTraits<identity::mojom::Captcha::DataView,
                    ::GoogleServiceAuthError::Captcha> {
  static std::string token(const ::GoogleServiceAuthError::Captcha& r) {
    return r.token;
  }
  static GURL audio_url(const ::GoogleServiceAuthError::Captcha& r) {
    return r.audio_url;
  }
  static GURL image_url(const ::GoogleServiceAuthError::Captcha& r) {
    return r.image_url;
  }
  static GURL unlock_url(const ::GoogleServiceAuthError::Captcha& r) {
    return r.unlock_url;
  }
  static int image_width(const ::GoogleServiceAuthError::Captcha& r) {
    return r.image_width;
  }
  static int image_height(const ::GoogleServiceAuthError::Captcha& r) {
    return r.image_height;
  }

  static bool Read(identity::mojom::CaptchaDataView data,
                   ::GoogleServiceAuthError::Captcha* out) {
    std::string token;
    GURL audio_url;
    GURL image_url;
    GURL unlock_url;
    if (!data.ReadToken(&token) || !data.ReadAudioUrl(&audio_url) ||
        !data.ReadImageUrl(&image_url) || !data.ReadUnlockUrl(&unlock_url) ||
        data.image_width() < 0 || data.image_height() < 0) {
      return false;
    }

    out->token = token;
    out->audio_url = audio_url;
    out->image_url = image_url;
    out->unlock_url = unlock_url;
    out->image_width = data.image_width();
    out->image_height = data.image_height();

    return true;
  }
};

template <>
struct StructTraits<identity::mojom::SecondFactor::DataView,
                    ::GoogleServiceAuthError::SecondFactor> {
  static std::string token(const ::GoogleServiceAuthError::SecondFactor& r) {
    return r.token;
  }
  static std::string prompt_text(
      const ::GoogleServiceAuthError::SecondFactor& r) {
    return r.prompt_text;
  }
  static std::string alternate_text(
      const ::GoogleServiceAuthError::SecondFactor& r) {
    return r.alternate_text;
  }
  static int field_length(const ::GoogleServiceAuthError::SecondFactor& r) {
    return r.field_length;
  }

  static bool Read(identity::mojom::SecondFactorDataView data,
                   ::GoogleServiceAuthError::SecondFactor* out) {
    std::string token;
    std::string prompt_text;
    std::string alternate_text;
    if (!data.ReadToken(&token) || !data.ReadPromptText(&prompt_text) ||
        !data.ReadAlternateText(&alternate_text) || data.field_length() < 0) {
      return false;
    }

    out->token = token;
    out->prompt_text = prompt_text;
    out->alternate_text = alternate_text;
    out->field_length = data.field_length();

    return true;
  }
};

}  // namespace mojo

#endif  // SERVICES_IDENTITY_PUBLIC_CPP_GOOGLE_SERVICE_AUTH_ERROR_STRUCT_TRAITS_H_
