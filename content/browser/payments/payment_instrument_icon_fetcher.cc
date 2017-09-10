// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/payments/payment_instrument_icon_fetcher.h"

#include "base/base64.h"
#include "base/bind_helpers.h"
#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/browser/storage_partition_impl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/manifest_icon_downloader.h"
#include "content/public/browser/manifest_icon_selector.h"
#include "content/public/common/manifest.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image.h"

namespace content {
namespace {

// TODO(zino): Choose appropriate icon size dynamically on different platforms.
// Here we choose a large ideal icon size to be big enough for all platforms.
// Note that we only scale down for this icon size but not scale up.
const int kPaymentAppIdealIconSize = 0xFFFF;
const int kPaymentAppMinimumIconSize = 0;

}  // namespace

void PaymentInstrumentIconFetcher::Start(
    scoped_refptr<ServiceWorkerContextWrapper> service_worker_context,
    const std::vector<payments::mojom::ImageObjectPtr>& image_objects,
    PaymentInstrumentIconFetcherCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  std::vector<Manifest::Icon> icons;
  for (const auto& image_object : image_objects) {
    Manifest::Icon icon;
    icon.src = image_object->src;
    icon.sizes.emplace_back(gfx::Size());
    icon.purpose.emplace_back(Manifest::Icon::IconPurpose::ANY);
    icons.emplace_back(icon);
  }

  GURL icon_url = ManifestIconSelector::FindBestMatchingIcon(
      icons, kPaymentAppIdealIconSize, kPaymentAppMinimumIconSize,
      Manifest::Icon::IconPurpose::ANY);

  if (icon_url.is_empty()) {
    std::move(callback).Run(std::string());
    return;
  }

  std::unique_ptr<std::vector<std::pair<int, int>>> provider_hosts =
      service_worker_context->GetProviderHostIds(icon_url.GetOrigin());

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&PaymentInstrumentIconFetcher::StartOnUI, icon_url,
                     std::move(provider_hosts), std::move(callback)));
}

void PaymentInstrumentIconFetcher::StartOnUI(
    const GURL& icon_url,
    std::unique_ptr<std::vector<std::pair<int, int>>> provider_hosts,
    PaymentInstrumentIconFetcherCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  for (const auto& host : *provider_hosts) {
    RenderFrameHostImpl* render_frame_host =
        RenderFrameHostImpl::FromID(host.first, host.second);
    if (!render_frame_host)
      continue;

    WebContentsImpl* web_content = static_cast<WebContentsImpl*>(
        WebContents::FromRenderFrameHost(render_frame_host));
    if (!web_content || web_content->IsHidden() ||
        icon_url.GetOrigin().spec().compare(
            web_content->GetLastCommittedURL().GetOrigin().spec()) != 0) {
      continue;
    }

    bool can_download_icon = ManifestIconDownloader::Download(
        web_content, icon_url, kPaymentAppIdealIconSize,
        kPaymentAppMinimumIconSize,
        base::Bind(&PaymentInstrumentIconFetcher::OnIconFetched,
                   base::Passed(std::move(callback))));
    if (can_download_icon)
      return;
  }

  // Failed to fetch icon.
  BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                          base::BindOnce(std::move(callback), std::string()));
}

void PaymentInstrumentIconFetcher::OnIconFetched(
    PaymentInstrumentIconFetcherCallback callback,
    const SkBitmap& bitmap) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (bitmap.drawsNothing()) {
    BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                            base::BindOnce(std::move(callback), std::string()));
    return;
  }

  gfx::Image decoded_image = gfx::Image::CreateFrom1xBitmap(bitmap);
  scoped_refptr<base::RefCountedMemory> raw_data = decoded_image.As1xPNGBytes();
  std::string encoded_data;
  base::Base64Encode(
      base::StringPiece(raw_data->front_as<char>(), raw_data->size()),
      &encoded_data);
  BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                          base::BindOnce(std::move(callback), encoded_data));
}

}  // namespace content
