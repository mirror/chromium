// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/decrypt_config.h"

#include <stddef.h>

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"

namespace media {

namespace {

std::string CipherModeAsString(EncryptionScheme::CipherMode cipher_mode) {
  switch (cipher_mode) {
    case EncryptionScheme::CIPHER_MODE_UNENCRYPTED:
      return "Unencrypted";
    case EncryptionScheme::CIPHER_MODE_AES_CTR:
      return "CTR";
    case EncryptionScheme::CIPHER_MODE_AES_CBC:
      return "CBC";
    default:
      return "Unknown";
  }
}

}  // namespace

DecryptConfig::DecryptConfig(const std::string& key_id,
                             const std::string& iv,
                             const std::vector<SubsampleEntry>& subsamples,
                             const EncryptionScheme& encryption_scheme)
    : key_id_(key_id),
      iv_(iv),
      subsamples_(subsamples),
      encryption_scheme_(encryption_scheme) {
  CHECK_GT(key_id.size(), 0u);
  CHECK(iv.size() == static_cast<size_t>(DecryptConfig::kDecryptionKeySize) ||
        iv.empty());
  DCHECK_EQ(iv.empty(), encryption_scheme_.mode() ==
                            EncryptionScheme::CIPHER_MODE_UNENCRYPTED);
}

DecryptConfig::~DecryptConfig() = default;

bool DecryptConfig::Matches(const DecryptConfig& config) const {
  if (key_id() != config.key_id() || iv() != config.iv() ||
      subsamples().size() != config.subsamples().size() ||
      !encryption_scheme_.Matches(config.encryption_scheme_)) {
    return false;
  }

  for (size_t i = 0; i < subsamples().size(); ++i) {
    if ((subsamples()[i].clear_bytes != config.subsamples()[i].clear_bytes) ||
        (subsamples()[i].cypher_bytes != config.subsamples()[i].cypher_bytes)) {
      return false;
    }
  }

  return true;
}

std::ostream& DecryptConfig::Print(std::ostream& os) const {
  os << "key_id:'" << base::HexEncode(key_id_.data(), key_id_.size()) << "'"
     << " iv:'" << base::HexEncode(iv_.data(), iv_.size()) << "'"
     << " cipher mode:" << CipherModeAsString(encryption_scheme_.mode())
     << " pattern:" << encryption_scheme_.pattern().encrypt_blocks() << ":"
     << encryption_scheme_.pattern().skip_blocks();

  os << " subsamples:[";
  for (const SubsampleEntry& entry : subsamples_) {
    os << "(clear:" << entry.clear_bytes << ", cypher:" << entry.cypher_bytes
       << ")";
  }
  os << "]";
  return os;
}

}  // namespace media
