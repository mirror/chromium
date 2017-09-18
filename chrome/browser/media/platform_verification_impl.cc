// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/platform_verification_impl.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "chrome/browser/media/cdm_storage_id.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

using media::mojom::PlatformVerification;

namespace {

// Only support version 1 of Storage Id. However, the "latest" version can also
// be requested.
const uint32_t kRequestLatestStorageIdVersion = 0;
const uint32_t kCurrentStorageIdVersion = 1;

}  // namespace

// static
void PlatformVerificationImpl::Create(
    content::RenderFrameHost* render_frame_host,
    media::mojom::PlatformVerificationRequest request) {
  DVLOG(2) << __FUNCTION__;
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(render_frame_host);

  // TODO(crbug.com/707335): Fix possible problem with timing issue where we
  // try to use the RenderFrameHost after it's destructed.
  mojo::MakeStrongBinding(
      base::MakeUnique<PlatformVerificationImpl>(render_frame_host),
      std::move(request));
}

PlatformVerificationImpl::PlatformVerificationImpl(
    content::RenderFrameHost* render_frame_host)
    : render_frame_host_(render_frame_host), weak_factory_(this) {
  DCHECK(render_frame_host);
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

void PlatformVerificationImpl::GetStorageId(uint32_t version,
                                            GetStorageIdCallback callback) {
  DVLOG(2) << __FUNCTION__;
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // Check that the request is for a supported version.
  if (version == kCurrentStorageIdVersion ||
      version == kRequestLatestStorageIdVersion) {
    chrome::ComputeStorageId(
        render_frame_host_,
        base::BindOnce(&PlatformVerificationImpl::OnStorageIdResponse,
                       weak_factory_.GetWeakPtr(), base::Passed(&callback)));
    return;
  }

  // Version not supported, so no Storage Id to return.
  std::move(callback).Run(version, std::vector<uint8_t>());
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
    DLOG(ERROR) << "Platform verification failed.";
    std::move(callback).Run(false, std::string(), std::string(), std::string());
    return;
  }

  DCHECK(!signed_data.empty());
  DCHECK(!signature.empty());
  DCHECK(!platform_key_certificate.empty());
  std::move(callback).Run(true, signed_data, signature,
                          platform_key_certificate);
}
#endif  // defined(OS_CHROMEOS)

void PlatformVerificationImpl::OnStorageIdResponse(
    GetStorageIdCallback callback,
    const std::vector<uint8_t>& storage_id) {
  std::move(callback).Run(kCurrentStorageIdVersion, storage_id);
}
