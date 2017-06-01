// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/download_item.h"

namespace content {

DownloadItem::RequestInfo::RequestInfo(std::vector<GURL> url_chain,
                                       GURL referrer_url,
                                       GURL site_url,
                                       GURL tab_url,
                                       GURL tab_referrer_url,
                                       std::string suggested_filename,
                                       base::FilePath forced_file_path,
                                       ui::PageTransition transition_type,
                                       bool has_user_gesture,
                                       std::string remote_address,
                                       base::Time start_time)
    : url_chain(url_chain),
      referrer_url(referrer_url),
      site_url(site_url),
      tab_url(tab_url),
      tab_referrer_url(tab_referrer_url),
      suggested_filename(suggested_filename),
      forced_file_path(forced_file_path),
      transition_type(transition_type),
      has_user_gesture(has_user_gesture),
      remote_address(remote_address),
      start_time(start_time) {}

DownloadItem::RequestInfo::RequestInfo() = default;

DownloadItem::RequestInfo::RequestInfo(const DownloadItem::RequestInfo& other) =
    default;

DownloadItem::RequestInfo::~RequestInfo() = default;

DownloadItem::DownloadItem(RequestInfo request_info)
    : request_info_(request_info) {}

const GURL& DownloadItem::GetURL() const {
  return request_info_.url_chain.empty() ? GURL::EmptyGURL()
                                         : request_info_.url_chain.back();
}

const std::vector<GURL>& DownloadItem::GetUrlChain() const {
  return request_info_.url_chain;
}

const GURL& DownloadItem::GetOriginalUrl() const {
  return request_info_.url_chain.empty() ? GURL::EmptyGURL()
                                         : request_info_.url_chain.front();
}

const GURL& DownloadItem::GetReferrerUrl() const {
  return request_info_.referrer_url;
}

const GURL& DownloadItem::GetSiteUrl() const {
  return request_info_.site_url;
}

const GURL& DownloadItem::GetTabUrl() const {
  return request_info_.tab_url;
}

const GURL& DownloadItem::GetTabReferrerUrl() const {
  return request_info_.tab_referrer_url;
}

std::string DownloadItem::GetSuggestedFilename() const {
  return request_info_.suggested_filename;
}

std::string DownloadItem::GetRemoteAddress() const {
  return request_info_.remote_address;
}

bool DownloadItem::HasUserGesture() const {
  return request_info_.has_user_gesture;
}

ui::PageTransition DownloadItem::GetTransitionType() const {
  return request_info_.transition_type;
}

const base::FilePath& DownloadItem::GetForcedFilePath() const {
  return request_info_.forced_file_path;
}

base::Time DownloadItem::GetStartTime() const {
  return request_info_.start_time;
}

}  // namespace content
