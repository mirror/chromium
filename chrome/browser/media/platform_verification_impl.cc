// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/platform_verification_impl.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

using media::mojom::PlatformVerification;

// static
void PlatformVerificationImpl::Create(
    content::RenderFrameHost* render_frame_host,
    media::mojom::PlatformVerificationRequest request) {
  DVLOG(2) << __FUNCTION__;
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(render_frame_host);
  mojo::MakeStrongBinding(
      base::MakeUnique<PlatformVerificationImpl>(render_frame_host),
      std::move(request));
}

PlatformVerificationImpl::PlatformVerificationImpl(
    content::RenderFrameHost* render_frame_host)
    : weak_factory_(this) {
  DCHECK(render_frame_host);
#if defined(OS_CHROMEOS)
  render_frame_host_ = render_frame_host;
#endif
}

PlatformVerificationImpl::~PlatformVerificationImpl() {}

void PlatformVerificationImpl::ChallengePlatform(
    const std::string& service_id,
    const std::string& challenge,
    ChallengePlatformCallback callback) {
  DVLOG(2) << __FUNCTION__;
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

#if defined(OS_CHROMEOS)
  if (!platform_verification_flow_.get())
    platform_verification_flow_ =
        base::MakeRefCounted<chromeos::attestation::PlatformVerificationFlow>();

  platform_verification_flow_->ChallengePlatformKey(
      content::WebContents::FromRenderFrameHost(render_frame_host_), service_id,
      challenge,
      base::Bind(&PlatformVerificationImpl::OnPlatformChallenged,
                 weak_factory_.GetWeakPtr(), base::Passed(&callback)));
#else
  // Not supported, so return failure.
  std::move(callback).Run(false, std::string(), std::string(), std::string());
#endif  // defined(OS_CHROMEOS)
}

void PlatformVerificationImpl::GetStorageId(GetStorageIdCallback callback) {
  // TODO(jrummell): Implement this. http://crbug.com/478960
  std::move(callback).Run(std::vector<uint8_t>());
}

#if defined(OS_CHROMEOS)
void PlatformVerificationImpl::OnPlatformChallenged(
    ChallengePlatformCallback callback,
    Result result,
    const std::string& signed_data,
    const std::string& signature,
    const std::string& platform_key_certificate) {
  DVLOG(2) << __FUNCTION__ << ": " << result;
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (result != chromeos::attestation::PlatformVerificationFlow::SUCCESS) {
    DCHECK(signed_data.empty());
    DCHECK(signature.empty());
    DCHECK(platform_key_certificate.empty());
    LOG(ERROR) << "Platform verification failed.";
    std::move(callback).Run(false, "", "", "");
    return;
  }

  DCHECK(!signed_data.empty());
  DCHECK(!signature.empty());
  DCHECK(!platform_key_certificate.empty());
  std::move(callback).Run(true, signed_data, signature,
                          platform_key_certificate);
}
#endif  // defined(OS_CHROMEOS)
