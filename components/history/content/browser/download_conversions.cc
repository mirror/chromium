// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/history/content/browser/download_conversions.h"

#include "base/logging.h"
#include "components/history/core/browser/download_constants.h"

namespace history {

content::DownloadItem::DownloadState ToContentDownloadState(
    DownloadState state) {
  switch (state) {
    case DownloadState::IN_PROGRESS:
      return content::DownloadItem::IN_PROGRESS;
    case DownloadState::COMPLETE:
      return content::DownloadItem::COMPLETE;
    case DownloadState::CANCELLED:
      return content::DownloadItem::CANCELLED;
    case DownloadState::INTERRUPTED:
      return content::DownloadItem::INTERRUPTED;
    case DownloadState::INVALID:
    case DownloadState::BUG_140687:
      NOTREACHED();
      return content::DownloadItem::MAX_DOWNLOAD_STATE;
  }
  NOTREACHED();
  return content::DownloadItem::MAX_DOWNLOAD_STATE;
}

DownloadState ToHistoryDownloadState(
    content::DownloadItem::DownloadState state) {
  switch (state) {
    case content::DownloadItem::IN_PROGRESS:
      return DownloadState::IN_PROGRESS;
    case content::DownloadItem::COMPLETE:
      return DownloadState::COMPLETE;
    case content::DownloadItem::CANCELLED:
      return DownloadState::CANCELLED;
    case content::DownloadItem::INTERRUPTED:
      return DownloadState::INTERRUPTED;
    case content::DownloadItem::MAX_DOWNLOAD_STATE:
      NOTREACHED();
      return DownloadState::INVALID;
  }
  NOTREACHED();
  return DownloadState::INVALID;
}

download::DownloadDangerType ToContentDownloadDangerType(
    DownloadDangerType danger_type) {
  switch (danger_type) {
    case DownloadDangerType::NOT_DANGEROUS:
      return download::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS;
    case DownloadDangerType::DANGEROUS_FILE:
      return download::DOWNLOAD_DANGER_TYPE_DANGEROUS_FILE;
    case DownloadDangerType::DANGEROUS_URL:
      return download::DOWNLOAD_DANGER_TYPE_DANGEROUS_URL;
    case DownloadDangerType::DANGEROUS_CONTENT:
      return download::DOWNLOAD_DANGER_TYPE_DANGEROUS_CONTENT;
    case DownloadDangerType::MAYBE_DANGEROUS_CONTENT:
      return download::DOWNLOAD_DANGER_TYPE_MAYBE_DANGEROUS_CONTENT;
    case DownloadDangerType::UNCOMMON_CONTENT:
      return download::DOWNLOAD_DANGER_TYPE_UNCOMMON_CONTENT;
    case DownloadDangerType::USER_VALIDATED:
      return download::DOWNLOAD_DANGER_TYPE_USER_VALIDATED;
    case DownloadDangerType::DANGEROUS_HOST:
      return download::DOWNLOAD_DANGER_TYPE_DANGEROUS_HOST;
    case DownloadDangerType::POTENTIALLY_UNWANTED:
      return download::DOWNLOAD_DANGER_TYPE_POTENTIALLY_UNWANTED;
    case DownloadDangerType::INVALID:
      NOTREACHED();
      return download::DOWNLOAD_DANGER_TYPE_MAX;
  }
  NOTREACHED();
  return download::DOWNLOAD_DANGER_TYPE_MAX;
}

DownloadDangerType ToHistoryDownloadDangerType(
    download::DownloadDangerType danger_type) {
  switch (danger_type) {
    case download::DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS:
      return DownloadDangerType::NOT_DANGEROUS;
    case download::DOWNLOAD_DANGER_TYPE_DANGEROUS_FILE:
      return DownloadDangerType::DANGEROUS_FILE;
    case download::DOWNLOAD_DANGER_TYPE_DANGEROUS_URL:
      return DownloadDangerType::DANGEROUS_URL;
    case download::DOWNLOAD_DANGER_TYPE_DANGEROUS_CONTENT:
      return DownloadDangerType::DANGEROUS_CONTENT;
    case download::DOWNLOAD_DANGER_TYPE_MAYBE_DANGEROUS_CONTENT:
      return DownloadDangerType::MAYBE_DANGEROUS_CONTENT;
    case download::DOWNLOAD_DANGER_TYPE_UNCOMMON_CONTENT:
      return DownloadDangerType::UNCOMMON_CONTENT;
    case download::DOWNLOAD_DANGER_TYPE_USER_VALIDATED:
      return DownloadDangerType::USER_VALIDATED;
    case download::DOWNLOAD_DANGER_TYPE_DANGEROUS_HOST:
      return DownloadDangerType::DANGEROUS_HOST;
    case download::DOWNLOAD_DANGER_TYPE_POTENTIALLY_UNWANTED:
      return DownloadDangerType::POTENTIALLY_UNWANTED;
    default:
      NOTREACHED();
      return DownloadDangerType::INVALID;
  }
}

download::DownloadInterruptReason ToContentDownloadInterruptReason(
    DownloadInterruptReason interrupt_reason) {
  return static_cast<download::DownloadInterruptReason>(interrupt_reason);
}

DownloadInterruptReason ToHistoryDownloadInterruptReason(
    download::DownloadInterruptReason interrupt_reason) {
  return static_cast<DownloadInterruptReason>(interrupt_reason);
}

uint32_t ToContentDownloadId(DownloadId id) {
  DCHECK_NE(id, kInvalidDownloadId);
  return static_cast<uint32_t>(id);
}

DownloadId ToHistoryDownloadId(uint32_t id) {
  DCHECK_NE(id, content::DownloadItem::kInvalidId);
  return static_cast<DownloadId>(id);
}

std::vector<content::DownloadItem::ReceivedSlice> ToContentReceivedSlices(
    const std::vector<DownloadSliceInfo>& slice_infos) {
  std::vector<content::DownloadItem::ReceivedSlice> result;

  for (const auto& slice_info : slice_infos)
    result.emplace_back(slice_info.offset, slice_info.received_bytes);

  return result;
}

std::vector<DownloadSliceInfo> GetHistoryDownloadSliceInfos(
    const content::DownloadItem& item) {
  std::vector<DownloadSliceInfo> result;

  for (const auto& slice : item.GetReceivedSlices())
    result.emplace_back(item.GetId(), slice.offset, slice.received_bytes);

  return result;
}

}  // namespace history
