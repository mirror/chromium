// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CDM_CDM_PROXY_H_
#define MEDIA_CDM_CDM_PROXY_H_

#include <stdint.h>

#include <memory>
#include <vector>

#include "base/callback.h"

namespace media {

// Key information structure containing data necessary to decrypt/decode media.
struct KeyInfo {
  KeyInfo();
  ~KeyInfo();
  // Crypto session for decryption.
  uint32_t crypto_session_id;
  // ID of the key.
  std::vector<uint8_t> key_id;
  // Opaque key blob for decrypting or decoding.
  std::vector<uint8_t> key_blob;
  // Indicates whether this key/key_id is usable.
  // The caller sets this to false to invalidate a key.
  bool is_usable_key;
};

// A proxy for the CDM.
class CdmProxy {
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

  enum Status {
    kOk,
    kFail,
  };

  enum Protocol {
    // Method using Intel CSME.
    KIntelCSME,
    // There will be more values in the future e.g. kD3D11RsaHardware,
    // kD3D11RsaSoftware to use the D3D11 RSA method.
  };

  enum FunctionId {
    // For intel hardware DRM path to call NegotiateCryptoSessionKeyExchange.
    kIntelHWDRMNegotiateCryptoSessionKeyExchange,
    // There will be more values in the future e.g. for D3D11 RSA method.
  };

  // Callback for Initialize().
  // If the proxy created a crypto session, then the ID for the
  // crypto session is |crypto_session_id|.
  using InitializeCB = base::OnceCallback<
      void(Status status, Protocol protocol, uint32_t crypto_session_id)>;

  // Callback for Process().
  // |output| is the output of processing.
  using ProcessCB =
      base::OnceCallback<void(Status status,
                              const std::vector<uint8_t>& output)>;

  // Callback for CreateMediaCryptoSessions().
  // |video_crypto_session_id| is set to a valid value if
  // |create_video_crypto_session| was set to true.
  // |audio_crypto_session_id| is set to a valid value if
  // |create_audio_crypto_session| was set to true.
  // |video_private_output_data| is set to a valid value if |video_private_data|
  // was non-null.
  // |audio_private_output_data| is set to a valid value if |audio_private_data|
  // was non-null.
  using CreateMediaCryptoSessionsCB =
      base::OnceCallback<void(Status status,
                              uint32_t video_crypto_session_id,
                              uint32_t audio_crypto_session_id,
                              uint64_t video_private_output_data,
                              uint64_t audio_private_output_data)>;
  explicit CdmProxy(std::unique_ptr<Client> client);
  virtual ~CdmProxy();

  // Initialize the proxy.
  // The status and the return values of the call is reported to
  // CdmProxyClient::OnInitializeResult().
  virtual void Initialize(InitializeCB init_cb) = 0;

  // Process and update the state of hte proxy with the data passed in.
  // The status and the return values of the call is reported to
  // CdmProxyClient::OnUpdateResult().
  // |expected_output_size| is the size of the output data.
  // Whether this value is used or not is implementation dependent,
  // but for some proxy implementations, the caller may have to specify this.
  virtual void Process(FunctionId function_id,
                       uint32_t crypto_session_id,
                       const std::vector<uint8_t>& input_data,
                       uint32_t expected_output_size,
                       ProcessCB process_cb) = 0;

  // Creates crypto sessions for handling media.
  // The status and the return values of the call is reported to
  // CdmProxyClient::OnSetupMediaCryptoSessionsResult().
  virtual void CreateMediaCryptoSessions(
      bool create_video_crypto_session,
      const std::vector<uint8_t>& video_private_data,
      bool create_audio_crypto_session,
      const std::vector<uint8_t>& audio_private_data,
      CreateMediaCryptoSessionsCB create_media_crypto_sessions_cb) = 0;

  // Send multiple key information to the proxy.
  virtual void SetKeyInfo(const std::vector<KeyInfo>& key_infos) = 0;

 protected:
  Client* client() { return client_.get(); }

 private:
  std::unique_ptr<Client> client_;
};

}  // namespace media

#endif  // MEDIA_CDM_CDM_PROXY_H_