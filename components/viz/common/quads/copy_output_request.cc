// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/quads/copy_output_request.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/trace_event/trace_event.h"
#include "components/viz/common/quads/copy_output_result.h"
#include "components/viz/common/quads/texture_mailbox.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace viz {

CopyOutputRequest::CopyOutputRequest(ResultFormat result_format,
                                     CopyOutputRequestCallback result_callback)
    : result_format_(result_format),
      result_callback_(std::move(result_callback)),
      scale_ratio_(1, 1) {
  DCHECK(!result_callback_.is_null());
  TRACE_EVENT_ASYNC_BEGIN0("viz", "CopyOutputRequest", this);
}

CopyOutputRequest::~CopyOutputRequest() {
  if (!result_callback_.is_null()) {
    // Send an empty result to indicate the request was never satisfied.
    SendResult(base::MakeUnique<CopyOutputResult>(result_format_, gfx::Rect()));
  }
}

void CopyOutputRequest::SetScaleRatio(int numerator, int denominator) {
  DCHECK_GT(numerator, 0);
  DCHECK_GT(denominator, 0);

  // Compute greatest common divisor of the two values. This is used to reduce
  // the many different scaling ratios across CopyOutputRequests into a smaller
  // canonical set, which benefits the copier implementations that need to cache
  // their image scaling pipeline setups.
  std::tuple<int, int> t{numerator, denominator};
  while (std::get<1>(t) != 0) {
    t = {std::get<1>(t), std::get<0>(t) % std::get<1>(t)};
  }
  const int divisor = std::get<0>(t);

  scale_ratio_ = {numerator / divisor, denominator / divisor};
}

void CopyOutputRequest::SendResult(std::unique_ptr<CopyOutputResult> result) {
  TRACE_EVENT_ASYNC_END1("viz", "CopyOutputRequest", this, "success",
                         !result->IsEmpty());
  if (result_task_runner_) {
    result_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(std::move(result_callback_), std::move(result)));
    result_task_runner_ = nullptr;
  } else {
    std::move(result_callback_).Run(std::move(result));
  }
}

void CopyOutputRequest::SetTextureMailbox(
    const TextureMailbox& texture_mailbox) {
  DCHECK_EQ(result_format_, ResultFormat::RGBA_TEXTURE);
  DCHECK(texture_mailbox.IsTexture());
  texture_mailbox_ = texture_mailbox;
}

// static
std::unique_ptr<CopyOutputRequest> CopyOutputRequest::CreateStubForTesting() {
  return base::MakeUnique<CopyOutputRequest>(
      ResultFormat::RGBA_BITMAP,
      base::BindOnce([](std::unique_ptr<CopyOutputResult>) {}));
}

}  // namespace viz
