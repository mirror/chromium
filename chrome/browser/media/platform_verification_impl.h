// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_PLATFORM_VERIFICATION_IMPL_H_
#define CHROME_BROWSER_MEDIA_PLATFORM_VERIFICATION_IMPL_H_

#include <string>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "content/public/browser/render_frame_host.h"
#include "media/mojo/interfaces/platform_verification.mojom.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/attestation/platform_verification_flow.h"
#endif

// Implements media::mojom::PlatformVerification. Can only be used on the
// UI thread because PlatformVerificationFlow lives on the UI thread.
class PlatformVerificationImpl : public media::mojom::PlatformVerification {
 public:
  static void Create(content::RenderFrameHost* render_frame_host,
                     media::mojom::PlatformVerificationRequest request);

  explicit PlatformVerificationImpl(
      content::RenderFrameHost* render_frame_host);
  ~PlatformVerificationImpl() override;

  // mojo::InterfaceImpl<PlatformVerification> implementation.
  void ChallengePlatform(const std::string& service_id,
                         const std::string& challenge,
                         ChallengePlatformCallback callback) override;
  void GetStorageId(uint32_t version, GetStorageIdCallback callback) override;

 private:
#if defined(OS_CHROMEOS)
  using Result = chromeos::attestation::PlatformVerificationFlow::Result;

  void OnPlatformChallenged(ChallengePlatformCallback callback,
                            Result result,
                            const std::string& signed_data,
                            const std::string& signature,
                            const std::string& platform_key_certificate);
#endif

  void OnStorageIdResponse(GetStorageIdCallback callback,
                           const std::vector<uint8_t>& storage_id);

  content::RenderFrameHost* render_frame_host_;

#if defined(OS_CHROMEOS)
  scoped_refptr<chromeos::attestation::PlatformVerificationFlow>
      platform_verification_flow_;
#endif

  base::WeakPtrFactory<PlatformVerificationImpl> weak_factory_;
};

#endif  // CHROME_BROWSER_MEDIA_PLATFORM_VERIFICATION_IMPL_H_
