// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_HISTORY_CONTENT_BROWSER_DOWNLOAD_CONVERSIONS_H_
#define COMPONENTS_HISTORY_CONTENT_BROWSER_DOWNLOAD_CONVERSIONS_H_

#include <stdint.h>

#include <string>

#include "components/download/public/common/download_danger_type.h"
#include "components/download/public/common/download_interrupt_reasons.h"
#include "components/history/core/browser/download_slice_info.h"
#include "components/history/core/browser/download_types.h"
#include "content/public/browser/download_item.h"

namespace history {

// Utility functions to convert between content::DownloadItem::DownloadState
// enumeration and history::DownloadState constants.
content::DownloadItem::DownloadState ToContentDownloadState(
    DownloadState state);
DownloadState ToHistoryDownloadState(
    content::DownloadItem::DownloadState state);

// Utility functions to convert between download::DownloadDangerType enumeration
// and history::DownloadDangerType constants.
download::DownloadDangerType ToContentDownloadDangerType(
    DownloadDangerType danger_type);
DownloadDangerType ToHistoryDownloadDangerType(
    download::DownloadDangerType danger_type);

// Utility functions to convert between download::DownloadInterruptReason
// enumeration and history::DownloadInterruptReason type (value have no
// meaning in history, but have a different type to avoid bugs due to
// implicit conversions).
download::DownloadInterruptReason ToContentDownloadInterruptReason(
    DownloadInterruptReason interrupt_reason);
DownloadInterruptReason ToHistoryDownloadInterruptReason(
    download::DownloadInterruptReason interrupt_reason);

// Utility functions to convert between content download id values and
// history::DownloadId type (value have no meaning in history, except
// for kInvalidDownloadId).
uint32_t ToContentDownloadId(DownloadId id);
DownloadId ToHistoryDownloadId(uint32_t id);

// Utility function to convert a history::DownloadSliceInfo vector into a
// vector of content::DownloadItem::ReceivedSlice.
std::vector<content::DownloadItem::ReceivedSlice> ToContentReceivedSlices(
    const std::vector<DownloadSliceInfo>& slice_infos);

// Construct a vector of history::DownloadSliceInfo from a
// content::DownloadItem object.
std::vector<DownloadSliceInfo> GetHistoryDownloadSliceInfos(
    const content::DownloadItem& item);

}  // namespace history

#endif  // COMPONENTS_HISTORY_CONTENT_BROWSER_DOWNLOAD_CONVERSIONS_H_
