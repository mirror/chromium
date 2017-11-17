// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CDM_CDM_PROXY_H_
#define MEDIA_CDM_CDM_PROXY_H_

#include <stdint.h>

#include <memory>
#include <vector>

#include "base/callback.h"
#include "media/base/media_export.h"

namespace media {

// Key information structure containing data necessary to decrypt/decode media.
struct MEDIA_EXPORT KeyInfo {
  KeyInfo();
  ~KeyInfo();
  // Crypto session for decryption.
  uint32_t crypto_session_id = 0;
  // ID of the key.
  std::vector<uint8_t> key_id;
  // Opaque key blob for decrypting or decoding.
  std::vector<uint8_t> key_blob;
  // Indicates whether this key/key_id is usable.
  // The caller sets this to false to invalidate a key.
  bool is_usable_key = true;
};

// A proxy for the CDM.
class MEDIA_EXPORT CdmProxy {
 public:
  // Client of the proxy.
  class Client {
   public:
    Client() {}
    virtual ~Client() {}
    // Called when there is a hardware reset and all the hardware context is
    // lost.
    virtual void NotifyHardwareReset() = 0;
  };

  enum class Status {
    kOk,
    kFail,
    kMaxStatus = kFail,
  };

  enum class Protocol {
    // Method using Intel CSME.
    kIntelConvergedSecurityAndManageabilityEngine,
    // There will be more values in the future e.g. kD3D11RsaHardware,
    // kD3D11RsaSoftware to use the D3D11 RSA method.
    kMaxProtocol = kIntelConvergedSecurityAndManageabilityEngine,
  };

  enum class Function {
    // For Intel hardware DRM path to call
    // ID3D11VideoContext::NegotiateCryptoSessionKeyExchange.
    kIntelHWDRMNegotiateCryptoSessionKeyExchange,
    // There will be more values in the future e.g. for D3D11 RSA method.
    kMaxFunction = kIntelHWDRMNegotiateCryptoSessionKeyExchange,
  };

  CdmProxy() {}
  virtual ~CdmProxy() {}

  // Callback for Initialize().
  // If the proxy created a crypto session, then the ID for the
  // crypto session is |crypto_session_id|.
  using InitializeCB = base::OnceCallback<
      void(Status status, Protocol protocol, uint32_t crypto_session_id)>;

  // Initialize the proxy.
  // The status and the return values of the call is reported to
  // |init_cb|.
  virtual void Initialize(Client* client, InitializeCB init_cb) = 0;

  // Callback for Process().
  // |output| is the output of processing.
  using ProcessCB =
      base::OnceCallback<void(Status status,
                              const std::vector<uint8_t>& output)>;

  // Process and update the state of the proxy.
  // The status and the return values of the call is reported to
  // |process_cb|.
  // |expected_output_size| is the size of the output data.
  // Whether this value is used or not is implementation dependent,
  // but for some proxy implementations, the caller may have to specify this.
  virtual void Process(Function function,
                       uint32_t crypto_session_id,
                       const std::vector<uint8_t>& input_data,
                       uint32_t expected_output_size,
                       ProcessCB process_cb) = 0;

  // Callback for CreateMediaCryptoSessions().
  // On suceess:
  // |video_crypto_session_id| is set to a valid value if
  // |create_video_crypto_session| was set to true.
  // |audio_crypto_session_id| is set to a valid value if
  // |create_audio_crypto_session| was set to true.
  // |video_private_output_data| is set to a valid value if |video_private_data|
  // was non-empty.
  // |audio_private_output_data| is set to a valid value if |audio_private_data|
  // was non-empty.
  using CreateMediaCryptoSessionsCB =
      base::OnceCallback<void(Status status,
                              uint32_t video_crypto_session_id,
                              uint32_t audio_crypto_session_id,
                              uint64_t video_private_output_data,
                              uint64_t audio_private_output_data)>;

  // Creates crypto sessions for handling media.
  // The status and the return values of the call is reported to
  // |create_media_crypto_sessions_cb|.
  // Set |create_video_crypto_session| to true if a video crypto session
  // should be created. Similar for |create_audio_crypto_session| but for audio.
  // If extra data has to be passed to further setup the video crypto session,
  // pass data to |video_private_data|. Similar for |audio_private_data|.
  virtual void CreateMediaCryptoSessions(
      bool create_video_crypto_session,
      const std::vector<uint8_t>& video_private_data,
      bool create_audio_crypto_session,
      const std::vector<uint8_t>& audio_private_data,
      CreateMediaCryptoSessionsCB create_media_crypto_sessions_cb) = 0;

  // Send multiple key information to the proxy.
  virtual void SetKeyInfo(const std::vector<KeyInfo>& key_infos) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(CdmProxy);
};

}  // namespace media

#endif  // MEDIA_CDM_CDM_PROXY_H_