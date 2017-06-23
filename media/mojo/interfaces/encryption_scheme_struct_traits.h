// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_INTERFACES_ENCRYPTION_SCHEME_STRUCT_TRAITS_H_
#define MEDIA_MOJO_INTERFACES_ENCRYPTION_SCHEME_STRUCT_TRAITS_H_

#include "media/base/encryption_scheme.h"
#include "media/mojo/interfaces/media_types.mojom.h"

namespace mojo {

template <>
struct StructTraits<media::mojom::PatternDataView,
                    media::EncryptionScheme::Pattern> {
  static uint32_t encrypt_blocks(
      const media::EncryptionScheme::Pattern& input) {
    return input.encrypt_blocks();
  }

  static uint32_t skip_blocks(const media::EncryptionScheme::Pattern& input) {
    return input.skip_blocks();
  }

  static bool Read(media::mojom::PatternDataView input,
                   media::EncryptionScheme::Pattern* output) {
    *output = media::EncryptionScheme::Pattern(input.encrypt_blocks(),
                                               input.skip_blocks());
    return true;
  }
};

template <>
struct StructTraits<media::mojom::EncryptionSchemeDataView,
                    media::EncryptionScheme> {
  static media::EncryptionScheme::CipherMode mode(
      const media::EncryptionScheme& input) {
    return input.mode();
  }

  static media::EncryptionScheme::Pattern pattern(
      const media::EncryptionScheme& input) {
    return input.pattern();
  }

  static bool Read(media::mojom::EncryptionSchemeDataView input,
                   media::EncryptionScheme* output) {
    media::EncryptionScheme::Pattern pattern;
    if (!input.ReadPattern(&pattern))
      return false;

    *output = media::EncryptionScheme(
        static_cast<media::EncryptionScheme::CipherMode>(input.mode()),
        pattern);

    return true;
  }
};

}  // namespace mojo

#endif  // MEDIA_MOJO_INTERFACES_ENCRYPTION_SCHEME_STRUCT_TRAITS_H_
