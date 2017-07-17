// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_MEDIA_SERVICE_CONNETOR_PROVIDER_IMPL_H_
#define CONTENT_BROWSER_RENDERER_HOST_MEDIA_SERVICE_CONNETOR_PROVIDER_IMPL_H_

#include <memory>

#include "content/common/content_export.h"
#include "media/capture/video/video_capture_jpeg_decoder.h"

namespace content {

// Implementation of ServiceConnectorProvider for the Browser process that
// establishes a connection via ServiceManagerConnection::GetForProcess().
class CONTENT_EXPORT ServiceConnectorProviderImpl
    : public media::ServiceConnectorProvider {
 public:
  void GetConnectorAsync(
      base::Callback<void(service_manager::Connector*)> callback) override;
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_MEDIA_SERVICE_CONNETOR_PROVIDER_IMPL_H_
