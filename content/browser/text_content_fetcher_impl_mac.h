// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_TEXT_CONTENT_FETCHER_IMPL_MAC_H_
#define CONTENT_BROWSER_TEXT_CONTENT_FETCHER_IMPL_MAC_H_

#include "content/public/browser/text_content_fetcher_mac.h"

#include "content/common/content_export.h"

class CONTENT_EXPORT TextContentFetcherImpl :
    public content::TextContentFetcher {

 public:
    void GetTextForSuggestions(
    const content::TextContentFetcher::ResponseCB& response_cb) override;

 private:
  // Callback for TextContentFetcherImpl::GetTextForSuggestions().
  /*void OnResponse(RetrieveCallback callback,
                  gfx::Range edit_range,
                  const std::string& text);*/
};


#endif // CONTENT_BROWSER_TEXT_CONTENT_FETCHER_IMPL_MAC_H_
